# Corne Practice Guide

A learning plan tailored to the keymap in `config/corne.keymap`. Work top to
bottom — don't jump ahead. Most people need 1–3 weeks to feel fluent, and the
home-row mods are the last thing to click. That's normal.

---

## Your Layout at a Glance

The Corne has 3 layers. You're always on the **Base** layer unless you hold a
thumb key.

### Base layer (default)

```
 TAB   Q   W   E   R   T        Y   U   I   O   P   BSPC
 ESC   A   S   D   F   G        H   J   K   L   ;   '
 SHFT  Z   X   C   V   B        N   M   ,   .   /   ESC
              GUI  LWR  SPC    ENT  RSE  ALT
```

- **TAB** (top-left): tap = Tab, hold = Raise layer.
- **ESC** (left of A): tap = Esc, hold = Ctrl.
- **LWR** (left thumb, middle): hold for the **Lower** layer (numbers/symbols).
- **RSE** (right thumb, middle): hold for the **Raise** layer (arrows/F-keys/BT).
- Thumbs: `GUI` `Lower` `Space` | `Enter` `Raise` `Alt`.

### Home-row mods (hold a home-row key + a key on the *other* hand)

```
 A = Gui    S = Alt    D = Shift   F = Ctrl
 J = Ctrl   K = Shift  L = Alt     ; = Gui
```

Tap them normally and they're just letters. **Hold + press a key on the
opposite hand** to use the modifier.

### Lower layer (hold left thumb)

```
 `   !   @   #   $   %        7   8   9   -   /   ESC
 ~   ^   &   *   (   )        4   5   6   +   *   BSPC
 |   \   [   ]   {   }        1   2   3   .   =   ENT
              GUI   -   SPC    ENT  -    0
```

Left hand = symbols. Right hand = number pad. `0` is the right thumb.

### Raise layer (hold right thumb)

```
 TAB  BT0 BT1 BT2 BT3 BT4     F1  F2  F3  F4  F5  BSPC
 CTL  CLR  -   -   -   -      LFT DWN UP  RGT  \  F12
 SHT   -   -   -   -   -      F6  F7  F8  F9  F10 F11
              GUI   -   SPC    ENT  -   ALT
```

Left hand = Bluetooth profile select (`BT0`–`BT4`) and `CLR` (clear pairing).
Right hand = arrow keys and function keys.

---

## Stage 1 — Base letters only (ignore mods for now)

Goal: muscle memory for letter positions on a columnar layout. Your fingers
move straight up/down, not staggered like a normal keyboard.

- Practice **slowly and accurately**, not fast. Speed comes later.
- Drill: `the quick brown fox jumps over the lazy dog` — repeat until smooth.
- Spend 10–15 min/day. Don't look down; trust the home row (F and J usually
  have bumps).

## Stage 2 — Thumbs and layers

- Practice **Space** and **Enter** on the thumbs until automatic.
- Hold **LWR** (left thumb) and type a phone number, then symbols.
- Hold **RSE** (right thumb) and move around with the arrow cluster
  (`LFT DWN UP RGT`).

## Stage 3 — Home-row mods (the big one)

The rule: **mod fires only when you hold a home-row key AND press a key on the
other hand.** Same-hand combos stay as letters.

- **Copy/paste:** hold `J` (Ctrl) + `C`/`V`. (J is right hand, C/V are left —
  cross-hand, so it works.)
- **Save:** hold `J` (Ctrl) + `S`.
- **Shift a letter:** hold `D` (Shift, left) + a right-hand letter like `J`.
- Drill capital letters: type `Hello World` using `D`/`K` for Shift.

If a mod ever "doesn't fire," you probably pressed too fast after another key
(the 150 ms idle guard) — pause a hair, then hold. If letters turn into mods by
accident, you're holding too long before the other key.

## Stage 4 — Put it together

Write real sentences with punctuation and capitals. Then real work: code,
emails, chat. Awkwardness here is just exposure — keep going.

---

## Recommended Practice Tools

- **keybr.com** — adaptive, teaches letters gradually. Best for Stage 1.
- **monkeytype.com** — clean typing test; turn on punctuation/numbers for later
  stages. Great for tracking speed/accuracy over time.
- **ngram practice / typing.io** — for coders; lots of symbols and brackets
  (good Stage 2–3 once you know the Lower layer).
- **keymapdb / your `keymap-drawer`** — print the layers and tape the cheat
  sheet near your screen for the first week.

## Tips

- **Don't chase WPM early.** Accuracy first; speed is a side effect.
- Expect a **dip then recovery** — you'll be slow for a few days, then it
  clicks.
- If home-row mods stay frustrating after a week, the timing is tunable in
  `config/corne.keymap` (`tapping-term-ms`, `require-prior-idle-ms`). Tell me
  what's misbehaving and I'll adjust it.
</content>
