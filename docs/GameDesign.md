# Game Design — `my_rpg`

> Owned by the **Game Director** agent. Living document — update as decisions
> land. Engineers read this to understand what features the game actually
> needs.

---

## 1. Identity

### 1.1 Working Title

**Starfall** — same as the engine on purpose. The engine exists to make this
game; both names share the central image. If a future second game ships on the
engine, this one gets a subtitle (e.g. "Starfall: The Long Year") and the
engine name stays bare.

### 1.2 One-Sentence Pitch

> *A sincere classical-fantasy RPG about a coastal village the morning after
> a falling star, where the world is almost the same — and the party you
> gather northward are the only people who remember what changed.*

### 1.3 Three Reference Points

1. **Final Fantasy I (1987)** — for the *shape*: classical fantasy, ancient
   cyclical evil, sincere world-saving stakes, named places that feel old.
2. **Dragon Slayer: The Legend of Heroes (1989)** — for the *structure*:
   kingdom-scale travel where *places* carry the narrative weight, not combat
   set-pieces; named NPCs you remember a year later.
3. **Chrono Trigger (1995)** — for the *tone and rhythm*: visible (not random)
   encounters, ensemble party assembled across the journey, time/echo motif,
   absolute commitment to sincerity. No irony, no winking.

### 1.4 The One Thing

**The world remembers wrong.**

Most of the world doesn't notice the star fell. People go about their lives
with small inconsistencies — a sign that says the wrong population number,
a recipe that's missing an ingredient nobody can name, a friend who's
slightly the wrong height. The player and the party are the small set of
people who can feel that something is *off*. Progress comes from noticing
what shouldn't be there (or shouldn't be missing), not from grinding levels.

Concretely: NPC dialogue, environmental flavor text, and item descriptions
shift slightly between visits in ways the player can catch. The "puzzle" of
the game is the world itself.

### 1.5 Tone Palette

**quiet · expectant · weathered · kind · exact**

- *quiet* — the music breathes, the dialogue doesn't shout.
- *expectant* — something is about to happen; the world is leaning forward.
- *weathered* — the world is old; people have lived here a long time.
- *kind* — characters are decent to each other by default. No edgelords.
- *exact* — descriptions are specific. Not "an old rope" — "salt-stiffened rope."

Any decision (line of dialogue, music cue, UI element) gets weighed against
these. Anything that contradicts (loud, frantic, ironic, cruel, vague) is
cut or revised.

---

## 2. World

### 2.1 Setting

A long coastline at the southern edge of a kingdom that has been declining
for a generation. The era reads as *late classical fantasy* — there are
roads and lighthouses and small books, but no industry. The northern
provinces are dying slowly; the southern villages persist because the sea
is still kind. People talk about "the Long Year" — the way the seasons
have stopped turning correctly since before most of them were born.

There is no empire to fight. There is no demon king on a throne. There is
only a world that is forgetting itself, and a star that has begun to fall
in pieces.

### 2.2 State At Game Start

A falling star landed in the northern hills the night before the game
opens. From the village of **Embercoast**, the southerly view is the same
as always; the northerly view is *almost* the same, but the lighthouse on
the cliff hasn't been lit, and nobody seems to find that strange except
the player and a few others.

The player wakes in their bed in Embercoast with the vague feeling of
having forgotten the previous evening. The village is going about its
morning. One person — Mina — looks at the player like she's checking
something.

### 2.3 The Three-Act Arc

- **Act 1 — Embercoast.** The player wakes, talks to the village, finds the
  road north blocked, and is given a reason to leave. The first companion
  joins. Tone: small, intimate, slightly off. *(v1 ships the opening of
  Act 1.)*

- **Act 2 — The Long Road.** Travel north through the dying provinces.
  Assemble a party of 3-4. Each companion has a personal reason for going
  north and a private theory about what's wrong. Tone: widening, lonelier,
  but warmer between party members.

- **Act 3 — The Place Where The Star Fell.** Reach the impact site. Confront
  what the star is (not a meteor — something cyclical, something that has
  fallen here before). The choice is not "fight or don't" but "what do you
  let it take from the world this time." Tone: still and weighty. Earned.

### 2.4 Key Locations

| Location           | Real Name      | Why It Exists                                              |
| ------------------ | -------------- | ---------------------------------------------------------- |
| `starter_town`     | **Embercoast** | First town. Coastal, weathered, pop. 47. The world in miniature: kind, careful, almost-right. |
| `cliff_lighthouse` | **The Salt Light** | The unlit lighthouse — first visible "something is wrong." Gates the road north. |
| `the_long_road`    | **The Long Road** | Travel region between Embercoast and the next major settlement. Act 2 territory. |
| `impact_site`      | **The Hollow** | Where the star landed. Act 3. Named only late. |

---

## 3. Characters

### 3.1 The Player Character

- **Default name:** **Iden** (player can rename at start, FF1/CT convention).
- **Background:** Born in Embercoast. Has lived there their whole life as far
  as anyone — including themself — can clearly remember. The night before the
  game, they were somewhere they cannot now place. They don't talk about it
  because they don't know how.
- **Voice:** Mostly silent (CT-style). NPCs address them by name. The few
  dialogue choices the player makes are short and direct, never witty.

### 3.2 NPCs (v1 Roster: 3 named)

#### Mina

- **Role:** First named NPC the player meets. Establishes the game's voice
  in three lines. The person who notices the player isn't quite themselves
  this morning.
- **Want:** She says she's worried about the lighthouse being unlit.
- **Need:** Someone to confirm she isn't the only one who feels the sky
  changed last night.
- **Voice:** Careful, observant, speaks the way someone speaks when they
  aren't sure they should be speaking at all. Short sentences. Trails off.
- **Sample dialogue:**
  - First meeting:
    > "Oh — you're up. You slept through it, then. You didn't see the sky."
  - Before player leaves Embercoast:
    > "I'm glad it wasn't only me. I was starting to think I'd dreamed it."
  - Repeat (after the player returns from the cliff):
    > "The light's still out. I don't know who's supposed to light it now."

#### Halor

- **Role:** The lighthouse keeper. The village's oldest resident. He gates
  the road north — not by refusing, but by giving the player a *reason* to
  go (Act 1 inciting incident). The first non-trivial conversation in the
  game.
- **Want:** He says he wants someone to climb the cliff and re-light the
  Salt Light because his knees won't take it anymore.
- **Need:** He needs the player specifically to go, because he suspects
  what the player is and doesn't want to say so yet.
- **Voice:** Older register. Speaks in observations, not opinions. Uses old
  words for things ("the watch," "the turning") without explaining them.
- **Sample dialogue:**
  - First meeting:
    > "You came down to the cliff yesterday. I saw you up there past
    > sunset. You don't remember, do you. That's all right. Some things
    > aren't ours to keep."
  - Granting the player the lighthouse key:
    > "Take this. The light's been kept by my family for four turnings.
    > You'll do for the fifth."

#### Wren

- **Role:** A child, maybe eight years old, playing alone behind the inn.
  Not central to Act 1 progression — optional but rewarding. Foreshadows
  the echo mechanic by saying things only the player can recognize as
  *wrong*.
- **Want:** To find her cat, which she insists ran north last night.
- **Need:** Someone to listen when she says her cat used to be grey, but
  this morning was always orange.
- **Voice:** Matter-of-fact, the way children are about things adults
  would equivocate over. Doesn't perform childishness.
- **Sample dialogue:**
  - First meeting:
    > "Mister, was Pepper grey or orange? Mum says he was always orange
    > but he wasn't. Was he."
  - After Mina conversation:
    > "If you go past the cliff you might see him. He went that way. I
    > saw him go."

---

## 4. The First Slice (v1 Scope)

### 4.1 What Ships In v1

- **Embercoast** — the full village, walkable, painted in the editor.
- **3 named NPCs** — Mina, Halor, Wren.
- **1 save point** — the Salt Light cairn at the foot of the cliff path
  (a small stone marker, not the lighthouse itself).
- **2 environmental detail moments:**
  1. A sign at the edge of town reading `EMBERCOAST · pop. 47 · take only what you can carry.`
  2. Wren's lost-cat poster on the inn wall, hand-drawn, which says the cat
     is **orange** even though Wren tells you it was grey.
- **1 inciting incident** — Halor's lighthouse-key conversation.
- **End state** — the player can take the cliff path north. The game ends
  the v1 slice when the player walks off the screen at the top of the cliff.

**Not in v1:** combat, inventory, party members, multiple maps, day/night,
weather, journal, options menu, title screen.

### 4.2 Intended Player Feeling

**Almost.**

The slice should make the player feel that the world is *almost* as it
should be, and they are *almost* who they should be, and the gap between
those two "almosts" is the reason to keep going.

If a playtester finishes the slice and says "it felt small but I want to
go north," it worked. If they say "it felt cute" or "it felt sad," it
missed.

### 4.3 Memorable Moments

1. **Mina's first line.** The first dialogue in the game. The player should
   feel: she sees something. About me. That I don't see.

2. **The signs that disagree.** The pop. 47 sign and Wren's poster vs.
   what Wren says. The first time the player notices the world *contradicts
   itself*. This is the seed of The One Thing.

3. **Halor giving the key.** The first time an NPC says something they
   clearly know more about than they're telling. Not mysterious — *kind*
   about it. "You'll do for the fifth."

---

## 5. Direction

### 5.1 Visual Direction

- **Color mood:** Muted dusk — soft purples, deep blues, amber lantern
  light. The world is not bright. Reference: late afternoon in coastal New
  England, not "Saturday morning JRPG."
- **Time of day in v1:** Pre-dawn shading into early morning. The game
  starts in blue-hour light; by the end of the slice the cliff path is
  catching sunrise.
- **Environmental detail density:** Moderate. Every screen has 2-4
  hand-placed details (a coiled rope, a bowl set out for a cat, a
  weather-bleached cloth on a line). Never empty, never busy.
- **Tile size:** 16×16. Pixel-art aesthetic, *not* modern HD-2D. Reads as
  SNES-era, deliberately.

### 5.2 Audio Direction

- **Music:** Acoustic-leaning chiptune. Think *Chrono Trigger*'s
  "Memories of Green" without the strings — solo woodwind or harp over a
  sparse pad. Loops should be long enough (~90s) to not exhaust the room.
- **SFX:** Warm and slightly synthesized. Footsteps are soft; dialogue
  ticks are gentle. No harsh frequencies.
- **Role of silence:** Large. The opening of the slice has *no music* for
  the first 30-60 seconds. The first time music plays is when the player
  steps outside their door. This gives the world room to be quiet.

---

## 6. Voice Bible

Ten example lines across registers. Anyone writing new text matches against
these. If a new line sounds like a different writer, one of them is wrong.

```
NPC dialogue (calm):
  "The tide's been odd lately. Comes in twice, goes out once."

NPC dialogue (worried):
  "You don't remember either, do you. I thought it was just me."

Item description:
  "A length of rope, salt-stiffened. Tied by a careful hand."

Save prompt:
  "Mark this moment?"

System message (path blocked):
  "Not yet."

Sign (environmental):
  "EMBERCOAST  ·  pop. 47  ·  take only what you can carry."

Title screen tagline:
  "What the star changed, and what it could not."

End-of-slice text:
  "There's more, north of here. You can feel it."

"You can't go that way":
  "Not yet."

"Saved.":
  "Held."
```

Voice rules:
- Short sentences. Trail off when uncertain.
- Specific nouns ("salt-stiffened rope") over abstract ones ("old rope").
- No exclamation marks in v1 except in Wren's dialogue (one per
  conversation, max).
- No questions mark-ending sentences if the line could land as a
  statement-trailing-off ("you don't remember either, do you.")

---

## 7. The "Is This In Scope?" Test

A proposed v1 feature ships only if it passes:

1. Does it serve the one-sentence pitch?
2. Does it reinforce at least one tone-palette adjective?
3. Can it be implemented in <2 weeks of engine + Lua work?
4. Can it be cut cleanly if it doesn't work?
5. Does it appear in at least one of the three memorable moments?

3 yes = maybe. 5 yes = ship. 4 yes = wait.

---

## 8. Cuts And Why

Decisions to *not* do something. Write a sentence; your future self will
try to add it back.

### Cut: Combat in v1 *(2026-06-16)*

- **What:** Any combat system — turn-based, real-time, anything.
- **Why:** The slice is about noticing the world, not fighting it. Combat
  in Act 1 would establish stakes the world doesn't yet have. FF1 has
  combat from minute one; this game doesn't, because the *threat* of this
  game isn't an enemy. It's the world forgetting.
- **When it returns:** Act 2, with the first party member joining.

### Cut: Day/Night Cycle in v1 *(2026-06-16)*

- **What:** A live time-of-day system.
- **Why:** The slice deliberately moves from pre-dawn to sunrise as a
  *scripted* arc. A live cycle would dilute the one moment the slice is
  built around (the sky changing as the player reaches the cliff).
- **When it returns:** Possibly Act 2 if travel days matter. Possibly
  never.

### Cut: Player Portrait *(2026-06-16)*

- **What:** A face graphic for the player character.
- **Why:** The player doesn't quite remember who they are. A fixed
  portrait commits to an answer the game wants to defer. Iden is described
  in NPC dialogue, not pictured.
- **When it returns:** Possibly Act 3, as a designed reveal.

---

## 9. Mapping To Engine Milestones

Every engine milestone produces a visible game improvement (per
`DesignDoc.md` §30). This is the contract.

| Engine Milestone        | Game Deliverable                                                           |
| ----------------------- | -------------------------------------------------------------------------- |
| ✅ M0 Bootstrap (2026-06-17) | Window opens on Windows (MSVC) and Linux (WSL2 + WSLg, g++-10). Title bar reads "Starfall." Not "my_rpg." |
| ✅ M1 Sprite Rendering (2026-06-17) | Placeholder Iden sprite visible at logical-center. Pre-dawn blue (#1A2440) clear. Verified Windows + Linux (WSL2/WSLg). |
| ✅ M2 Input & Movement (2026-06-18) | Iden walks 4-directionally on WASD/arrows with most-recent-axis-wins resolution. TimeFantasy chara2 slot 1 (with placeholder fallback). 60 logical px/s default. Verified Windows + Linux. |
| ✅ M2.5 Sprite Animation (2026-06-18) | Iden faces direction of travel via 4-directional sprite swap. Walk cycle (`[walk1, stand, walk2, stand]`, 8 fps) plays when moving; idle stand frame when standing. AnimatedSprite primitive added to engine_render. Verified Windows + Linux (26/26 tests pass). |
| ✅ M2.75 Audio (2026-06-18) | Opening silence. First movement triggers a 2s fade-in of the placeholder Embercoast theme. Footstep SFX every 16 logical px (~2/sec). AudioSystem + Music/Sound + 16 SFX tracks via SDL3_mixer 3.2.4. Verified Windows + Linux (36/36 tests pass). |
| M3 Tilemap & Collision  | Embercoast traversable on a hand-coded map. Cliff path blocked.            |
| M3.25 Tileset Animation | Ocean tiles lap and lanterns flicker at Embercoast.                        |
| M3.5 Camera Follow      | Iden walks the full Embercoast map. Camera follows, clamps to map bounds.  |
| M4 Scene File Format    | `embercoast.scene.json` loadable; replaces the hand-coded version.         |
| M5 Editor v1            | The room can be rearranged in the editor without code.                     |
| M6 Lua Scripting        | Mina exists as a `ScriptComponent`. She says her first line.               |
| M7 Dialogue             | All three NPCs playable. The slice is end-to-end traversable.              |
| M8 Save System          | The Salt Light cairn save point works; state persists.                     |
| M9 Packaging            | A friend downloads the slice and plays it. Reports the *Almost* feeling.   |
| M10 Steam Prep          | Controller support. Slice ready for a store-page draft.                    |

If a game deliverable falls behind, engine work *pauses* until it catches up.

### Beyond v1 — Phase 2 (Ship Polish) and Phase 3 (Acts 2–3)

The v1 slice ships at M10. Acts 2–3 + general engine polish are tracked
under **Phase 2** (M11 UI Framework, M12 Pause + Settings, M13 Map
Transitions, M14 Asset Packer, M15 Accessibility, M16 Particles) and
**Phase 3** (M17 Inventory, M18–M19 Combat, M20 Party, M21 Quests, M22
Cutscenes). Game-deliverable mappings for each are owned by the Game
Director and filled in as those milestones come into focus. Full
breakdown lives in `docs/DesignDoc.md` §22.A.

---

## 10. Open Questions

Things the Game Director hasn't decided yet.

- [ ] Does Iden have a fixed gender, or is it player-pickable? *(Lean: unspecified, NPCs use "you" exclusively. Defer.)*
- [ ] Does the rename-at-start exist in v1, or is the player called Iden? *(Lean: Iden only in v1. Renaming is a v1.5 polish task.)*
- [ ] First companion's name and shape — joins at the top of the cliff. *(Defer past v1.)*
- [ ] The kingdom's name. *(Not needed for v1. Halor never says it.)*
- [ ] Does Wren's cat appear in v1, or only her poster? *(Lean: poster only. The cat is a future hook.)*
- [ ] Soundtrack composer. *(Defer.)*
- [ ] Whether the "echo" inconsistencies between visits (NPC remembers a line differently the second time) ship in v1 or land in v2. *(Lean: ship one — Wren's grey-vs-orange. Defer the system to v2.)*
