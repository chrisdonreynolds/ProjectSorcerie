"""Set unique, class-themed placeholder SpellName/Description on every row of
/Game/GameplayAbilities/SpellData/SpellData, touching ONLY those two fields.

Run in the Unreal Editor Python console / Output Log:
    py Content/Python/aura_spelldata_placeholder_names.py

Why export/import instead of a per-row struct read-modify-write:
  Unreal's Python API exposes NO per-row struct get/set for DataTables whose row
  struct is a custom UScriptStruct. GetDataTableRowFromName is
  BlueprintInternalUseOnly + CustomThunk with an FTableRowBase& parameter, so it
  is not usable from Python (its declared type truncates to the empty base struct).
  The only lossless route is: export the whole table to JSON -> deep-copy it ->
  change only SpellName/Description per row -> re-import the SAME json.
  All other fields are re-fed verbatim from the table's own export, and the script
  then re-exports and diffs every non-target field of every row against the backup.
  The original JSON is written to Saved/SpellData_backup.json before any mutation.
"""
import json
import os

DT_PATH = "/Game/GameplayAbilities/SpellData/SpellData"

CLASS_ORDER = ["Fire", "Water", "Earth", "Light", "Dark", "Ghostfire", "Beasts",
               "Lightning", "Cosmic", "Blood", "Plants", "Fae", "Ice", "Wind", "Poison"]

# Per-class word pools. Prefixes are unique to one class and nouns are unique
# inside a class, so prefix+noun combinations are unique across the whole table
# (36 combos per class; a class would need >36 rows to ever need a suffix).
POOLS = {
    "Fire": (("Cinder", "Ember", "Ash", "Pyre", "Scorch", "Magma"),
             ("spike", "lash", "bloom", "fang", "veil", "surge")),
    "Water": (("Tidal", "Brine", "Torrent", "Coral", "Wave", "Abyss"),
              ("call", "grasp", "ward", "echo", "lament", "spout")),
    "Earth": (("Stone", "Boulder", "Terra", "Quake", "Granite", "Root"),
              ("shatter", "grip", "bulwark", "slam", "maw", "tremor")),
    "Light": (("Radiant", "Dawn", "Halo", "Solar", "Lumen", "Gleam"),
              ("flare", "verdict", "lance", "crown", "blessing", "beam")),
    "Dark": (("Umbral", "Shade", "Night", "Void", "Dusk", "Obsidian"),
             ("grasp", "curse", "mire", "shroud", "harvest", "toll")),
    "Ghostfire": (("Wisp", "Phantom", "Spectre", "Soul", "Pale", "Hollow"),
                  ("pyre", "wail", "cinder", "lantern", "veil", "flame")),
    "Beasts": (("Feral", "Primal", "Wild", "Talon", "Fang", "Horn"),
               ("call", "charge", "frenzy", "pack", "howl", "pounce")),
    "Lightning": (("Storm", "Thunder", "Arc", "Volt", "Tempest", "Static"),
                  ("strike", "chain", "lash", "shock", "surge", "crack")),
    "Cosmic": (("Astral", "Stellar", "Nebula", "Celestial", "Meteor", "Quasar"),
               ("rift", "fall", "sigil", "crown", "orbit", "judgment")),
    "Blood": (("Crimson", "Sanguine", "Vein", "Gore", "Rust", "Scarlet"),
              ("offering", "pact", "spill", "bloom", "rite", "thirst")),
    "Plants": (("Verdant", "Thorn", "Bloom", "Bramble", "Moss", "Petal"),
               ("lash", "snare", "blossom", "ward", "spine", "garden")),
    "Fae": (("Pixie", "Glimmer", "Elfin", "Twilight", "Moth", "Dream"),
            ("charm", "hex", "dance", "circle", "bloom", "mischief")),
    "Ice": (("Frost", "Glacier", "Rime", "Sleet", "Hail", "Snow"),
            ("lance", "shard", "prison", "lantern", "bloom", "fang")),
    "Wind": (("Gale", "Zephyr", "Squall", "Whisper", "Cyclone", "Breeze"),
             ("step", "lash", "song", "dash", "spiral", "ward")),
    "Poison": (("Venom", "Toxic", "Bile", "Adder", "Blight", "Miasma"),
               ("spit", "cloud", "thorn", "brew", "coil", "fume")),
    # Only used if a row's Class does not resolve to one of the 15 known classes.
    "OTHER": (("Runed", "Warded", "Sigil", "Arcane", "Woven", "Bound"),
              ("mark", "cantrip", "glyph", "rite", "verse", "seal")),
}

# One sentence per class; the spell name is embedded so no two rows ever share
# a description. The per-class counter cycles the phrasing between spells.
DESCS = {
    "Fire": ["Conjures {n}, a searing mote of flame that bursts on impact and leaves the target burning.",
             "Releases {n}, scorching everything in a short arc and applying burning damage over time.",
             "Ignites the air into {n}, a rolling wave of fire that keeps smouldering where it lands.",
             "Detonates {n} at the point of impact, immolating nearby foes for a brief burn.",
             "Forges {n} from living embers; the flames cling to whatever they touch."],
    "Water": ["Summons {n}, a crushing surge that soaks the ground and slows anyone standing in it.",
              "Calls down {n}, a spiralling torrent that batters the target and pushes them back.",
              "Draws {n} from the deep, drenching the area and leaving foes heavy and chilled.",
              "Unleashes {n}, a high-pressure jet that pierces armour and saps momentum.",
              "Forms {n}, a churning pool that drags enemies down as it drains their footing."],
    "Earth": ["Splits the ground into {n}, a jagged ridge that shatters anything caught on its crest.",
              "Raises {n}, a slab of living stone that slams down and staggers the target.",
              "Channels {n}, sending a shudder through the earth that knocks foes from their feet.",
              "Hardens soil into {n}, a battering weight that breaks bone and guard alike.",
              "Grinds stone into {n}, a slow vise that traps and crushes whatever stands within."],
    "Light": ["Casts {n}, a beam of pure light that sears flesh and pierces shadow.",
              "Incants {n}, bathing the field in radiance that scorches the unclean.",
              "Summons {n}, an arc of dawn-bright energy that cuts cleanly through the dark.",
              "Raises {n}, a halo that flares outwards and blinds those who look upon it.",
              "Focuses sunlight into {n}, a concentrated flare that burns away corruption."],
    "Dark": ["Invokes {n}, a pool of devouring shadow that gnaws at life and light alike.",
             "Weaves {n}, a creeping gloom that clings to the target and withers their strength.",
             "Opens {n}, a void-touched curse that drains vigour from those it touches.",
             "Speaks {n}, and the shadows answer, coiling around the victim and stealing their breath.",
             "Unravels {n}, a mire of blackest night that swallows sound and warmth."],
    "Ghostfire": ["Releases {n}, a pale spirit-flame that burns the soul rather than the flesh.",
                  "Summons {n}, wisps of cold fire that haunt the target long after they land.",
                  "Lights {n}, a lantern of spectral flame that sears the living and comforts the dead.",
                  "Calls forth {n}, a wailing pyre that clings to the victim and will not be shaken.",
                  "Kindles {n}, a flickering grave-light that scorches all it drifts across."],
    "Beasts": ["Calls {n}, a primal charge that tears through the target with claw and fury.",
               "Invokes {n}, rousing a pack instinct that savages anything nearby.",
               "Unleashes {n}, a feral pounce that knocks the victim down and rends them.",
               "Sounds {n}, a hunting howl that drives the caster into a frenzied assault.",
               "Bonds with {n}, granting the ferocity of a cornered beast for a short while."],
    "Lightning": ["Lashes out with {n}, a forking bolt that arcs between every foe it can reach.",
                  "Calls {n}, a thunderclap that strikes instantly and leaves ears ringing.",
                  "Charges the air into {n}, a crackling discharge that leaps from target to target.",
                  "Unloads {n}, a storm-front of static that shocks and briefly stuns.",
                  "Grounds {n} through the caster, releasing a brilliant, deafening strike."],
    "Cosmic": ["Opens {n}, a rift of starlight that scours the target with alien radiance.",
               "Calls down {n}, a falling star that detonates with celestial force.",
               "Inscribes {n}, a constellation sigil that burns the unworthy who cross it.",
               "Aligns the heavens into {n}, a beam of judgement from a distant star.",
               "Bends space into {n}, collapsing a mote of void onto the target."],
    "Blood": ["Offers {n}, spilling the caster's own vitality to empower a crimson strike.",
              "Seals {n}, a sanguine pact that bleeds the target to mend the caster.",
              "Hurls {n}, a clot of living blood that clings and festers on impact.",
              "Inscribes {n}, a blood rite that siphons life from everything it marks.",
              "Wets the ground with {n}, a scarlet bloom that drains all who stand in it."],
    "Plants": ["Grows {n}, a whip of thorned vine that lashes and holds the target fast.",
               "Sows {n}, a bramble snare that entangles anything that steps across it.",
               "Wakes {n}, a field of blossoms that mends allies and poisons trespassers.",
               "Raises {n}, a flowering ward whose petals harden into biting spines.",
               "Cultivates {n}, a rapid garden that buries its roots in the target's flesh."],
    "Fae": ["Whispers {n}, a glamour that charms the target into lowering their guard.",
            "Dances {n}, a ring of moonlit motes that hexes whoever enters it.",
            "Sprinkles {n}, fey dust that bewilders foes and mends the caster's friends.",
            "Stitches {n}, a mischievous knot of magic that ties a foe's fate to their mistake.",
            "Sings {n}, a twilight lullaby that lulls the target and leaves them exposed."],
    "Ice": ["Hurls {n}, a lance of frost that skewers the target and freezes them in place.",
            "Shatters {n}, a burst of shards that shreds and slows everything it touches.",
            "Seals the target in {n}, a prison of black ice that must be broken from within.",
            "Lights {n}, a pale lantern whose cold bloom numbs and slows the limbs.",
            "Buries the field under {n}, a creeping glacier that chills all who linger."],
    "Wind": ["Sends {n}, a cutting gale that slices the target and shoves them off balance.",
             "Rides {n}, a zephyr step that carries the caster past enemies in a blur.",
             "Channels {n}, a whirling squall that scatters projectiles and buffets foes.",
             "Sings {n}, a humming breeze that hastens allies and slices whatever it grazes.",
             "Looses {n}, a spiralling updraft that lifts and hurls the target away."],
    "Poison": ["Spits {n}, a corrosive glob that eats through armour and leaves creeping venom.",
               "Raises {n}, a cloud of toxic spores that lingers and poisons all who breathe it.",
               "Coat {n} onto the caster's strikes, adding a withering blight to every blow.",
               "Brews {n}, a bubbling miasma that rots flesh and saps the victim's stamina.",
               "Coils {n} around the target, a serpent's bite that spreads venom through the blood."],
    "OTHER": ["Traces {n}, a scrawled sigil whose magic answers the caster's intent.",
              "Channels {n}, an unaligned rite that strikes with raw, undirected power.",
              "Locks {n} into place, a warding glyph that reacts to whatever it meets.",
              "Releases {n}, a bound cantrip that carries no element and strikes true regardless.",
              "Scribes {n}, a verse of raw mana that behaves exactly as the wielder commands."],
}


def find_key(d, base):
    """JSON may use the friendly authored name or the raw mangled one."""
    if base in d:
        return base
    for k in d.keys():
        if k.startswith(base + "_"):
            return k
    return None


def norm_class(v):
    if v is None:
        return None
    s = str(v)
    if "::" in s:
        s = s.split("::")[-1]
    s = s.strip()
    if s.isdigit():
        i = int(s)
        return CLASS_ORDER[i] if 0 <= i < len(CLASS_ORDER) else None
    return s


def to_rows(data):
    """Normalise exported JSON to {row_name: row_object}."""
    if isinstance(data, dict):
        return data
    out = {}
    for item in data:
        out[str(item.get("Name", item.get("name")))] = item
    return out


table = unreal.EditorAssetLibrary.load_asset(DT_PATH)
if table is None:
    raise RuntimeError("Could not load " + DT_PATH)

row_names = [str(n) for n in unreal.DataTableFunctionLibrary.get_data_table_row_names(table)]
print("ROWS_BEFORE:", len(row_names))

before_json = unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table)
if not before_json:
    raise RuntimeError("export_data_table_to_json_string returned None - aborting, nothing changed")

saved_dir = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())
backup_path = os.path.join(saved_dir, "SpellData_backup.json")
with open(backup_path, "w", encoding="utf-8") as fh:
    fh.write(before_json)
print("BACKUP_WRITTEN:", backup_path)

backup_rows = to_rows(json.loads(before_json))
work_root = json.loads(before_json)
work_rows = to_rows(work_root)

counters = dict((c, 0) for c in CLASS_ORDER)
used = set()
failures = []
renamed = 0
rescribed = 0
info = []
unknown_classes = set()

for rn in sorted(row_names):
    row = work_rows.get(rn)
    brow = backup_rows.get(rn)
    if not isinstance(row, dict) or not isinstance(brow, dict):
        failures.append((rn, "row missing or not an object in JSON export"))
        continue
    nk = find_key(row, "SpellName")
    dk = find_key(row, "Description")
    ck = find_key(row, "Class")
    if not nk or not dk:
        failures.append((rn, "SpellName/Description key not found"))
        continue
    cls = norm_class(row.get(ck)) if ck else None
    if cls not in POOLS:
        if cls:
            unknown_classes.add(str(cls))
        cls = "OTHER"
    idx = counters[cls]
    counters[cls] = idx + 1
    prefixes, nouns = POOLS[cls]
    base = prefixes[(idx // len(nouns)) % len(prefixes)] + nouns[idx % len(nouns)]
    name = base
    n = 1
    while name in used:
        n += 1
        name = "%s%d" % (base, n)
    used.add(name)
    desc = DESCS[cls][idx % len(DESCS[cls])].format(n=name)
    old_name, old_desc = brow.get(nk), brow.get(dk)
    row[nk] = name
    row[dk] = desc
    if str(old_name) != name:
        renamed += 1
    if str(old_desc) != desc:
        rescribed += 1
    info.append((rn, cls, str(row.get(ck)), name, desc))

table.modify()
import_ok = unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(work_root))

after_names = [str(n) for n in unreal.DataTableFunctionLibrary.get_data_table_row_names(table)]
after_json = unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table)
after_rows = to_rows(json.loads(after_json)) if after_json else {}

# Prove preservation: every field except SpellName/Description must be identical.
missing = [rn for rn in row_names if rn not in after_rows]
extra = [rn for rn in after_rows.keys() if rn not in row_names]
diff_count = 0
diff_examples = []
for rn in sorted(row_names):
    b, a = backup_rows.get(rn), after_rows.get(rn)
    if not isinstance(b, dict) or not isinstance(a, dict):
        continue
    for k in sorted(set(list(b.keys()) + list(a.keys()))):
        if k.startswith("SpellName") or k.startswith("Description"):
            continue
        if b.get(k) != a.get(k):
            diff_count += 1
            if len(diff_examples) < 6:
                diff_examples.append((rn, k, b.get(k), a.get(k)))


def ffw(src, label):
    """FFW is the one row with genuinely non-placeholder data - use as control."""
    r = src.get("FFW")
    if not isinstance(r, dict):
        print(label, "FFW_MISSING")
        return
    print(label, "FFW ImpactVFX=%s EffectsEnemy_len=%s" % (
        r.get(find_key(r, "ImpactVFX")), len(r.get(find_key(r, "EffectsEnemy")) or [])))


print("IMPORT_OK:", import_ok, "ROWS_AFTER:", len(after_names))
ffw(backup_rows, "BEFORE")
ffw(after_rows, "AFTER")
print("RENAMED:", renamed, "REDESCRIBED:", rescribed, "UNIQUE_NAMES:", len(used))
print("PRESERVED_FIELD_DIFFS:", diff_count, diff_examples)
print("MISSING_ROWS:", missing[:10], "EXTRA_ROWS:", extra[:10])
print("UNKNOWN_CLASSES:", sorted(unknown_classes))

step = max(1, len(info) // 8)
for s in info[::step][:8]:
    print("SAMPLE:", s[0], "|", s[1], "|", s[3], "|", s[4])
for f in failures[:10]:
    print("FAILED:", f[0], f[1])

if not import_ok or missing or extra or len(after_names) != len(row_names):
    print("ERROR: structural verification failed - restoring original JSON from backup")
    table.modify()
    unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, before_json)
    print("RESTORED_ROWS:", len(unreal.DataTableFunctionLibrary.get_data_table_row_names(table)))
else:
    unreal.EditorAssetLibrary.save_asset(DT_PATH, only_if_is_dirty=False)
    print("SAVED:", DT_PATH)
