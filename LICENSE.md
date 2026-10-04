# Licensing

FxmeFX is a set of JUCE plugins built on [FxmeTools](https://github.com/odoare/FxmeTools)
(vendored as the `lib/FxmeTools` submodule), which is itself split into a
framework-free half and a JUCE half under different licences. This document
explains which terms apply to FxmeFX and why, mirroring
`lib/FxmeTools/LICENSE.md`.

```
Everything in Source/ and cmake/   AGPL-3.0-or-later, or commercial terms
Source/PdCommon/m_pd.h             Pure Data's own licence (see below)
The impulse responses              their authors' terms (see below)
```

A file that states another licence in its own header (as an SPDX identifier)
follows that; every other file in this repository is under the terms of the
first line.

## The plugins — AGPL-3.0-or-later, or commercial

Every effect (its DSP class, its GUI component, its processor and editor) and
the shared code in `Source/Common` compile against JUCE and against
`lib/FxmeTools/FxmeTools/`, FxmeTools' own JUCE module. JUCE 8 is itself
dual-licensed (AGPLv3, or a commercial JUCE licence), and FxmeTools' JUCE
module mirrors that shape rather than fighting it. Distributing FxmeFX, or any
of its effects inside another plugin, therefore means one of:

    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial

- **Under the AGPLv3** (full text in `LICENSE`): use, modify and distribute
  freely, including as a hosted service, provided the complete corresponding
  source is offered to anyone who receives the plugin (or uses it over a
  network). This pairs with using JUCE under its own AGPLv3 option.
- **Under commercial terms**: available from the author for anyone holding a
  commercial JUCE licence who does not wish to release under the AGPL.
  `LicenseRef-FXME-Commercial` refers to the same commercial terms FxmeTools
  itself offers; contact via [github.com/odoare](https://github.com/odoare) or
  www.fx-mechanics.com. A commercial grant here does not include a JUCE
  licence; that is separate, from Raw Material Software.

The Pure Data externals are in this half too. They are not the "FxmeCore
only" case `lib/FxmeTools/LICENSE.md` describes (a tool that links only the
framework-free half and needs only the LGPL): each external hosts the effect's
JUCE `AudioProcessor` headlessly and links the JUCE modules it needs, so it
derives from JUCE like the plugins do.

The effects' factory presets (`Source/<Effect>/Presets`) and the build files
(`CMakeLists.txt`, `cmake/`, `.github/`) are under the same terms.

## Earlier releases

FxmeFX releases up to and including 0.3.0 were published under the GNU LGPL
3.0. Those releases, and copies made of them, keep the terms they were
published under. The terms above apply from 0.4.0 on, the first release built
on the reworked FxmeTools licensing.

## Third-party material

- **`Source/PdCommon/m_pd.h`** is Pure Data's external API header, copyright
  Miller Puckette and others, under Pure Data's own licence (a BSD-style
  licence, the `LICENSE.txt` of the Pure Data distribution). It is included so
  the externals build without a Pure Data checkout.
- **The built-in impulse responses** (`Source/ConvolReverb/ir`,
  `Source/Cab/IR`) are recordings by third parties, embedded in the plugins
  under their authors' terms. The cabinet IRs come from free IR packs (see
  `Source/Cab/IR/Readme.txt`); the authors of the reverb IRs are to be listed
  here with their terms.
- **`lib/FxmeTools/WDL/`** is a submodule from Cockos Incorporated under the
  zlib licence. It supplies the convolution engine of ConvolReverb and Cab.

The shared FX-Mechanics code itself lives in the `lib/FxmeTools` submodule
under that repository's `LICENSE.md`: its `core/` half is
`LGPL-3.0-or-later`, its `FxmeTools/` JUCE module AGPL/commercial on the same
terms as above.

---

Copyright (c) 2023-2026 Olivier Doaré.
