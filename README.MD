Foobar2000 x64 plugin for reading video game archives without unpacking.

## Supported formats

- **Cyberpunk 2077**: `.archive` (limited to `.wem` contents)
- **GTA IV**: `.rpf`
- **GTA V**: `.rpf`
- **TDU 2**: `.big` + `.map`, `.bnk`

## Usage

[foo_input_vgmstream](https://www.foobar2000.org/components/view/foo_input_vgmstream) is required for audio decoding.

foo_input_ffmpeg is **not recommended**, as it extracts archives to a temp directory instead of reading contents in place.

"Extract..." option is available in the playlist context menu for archive entries.

### GTA IV & GTA V

An [AES decryption key](https://gtamods.com/wiki/Cryptography#Key) must be entered in the preferences or loaded from the game's exe file.

### Filename hashes

The plugin comes with lists of original filenames in order to reverse the hashes stored in the archives. A list may be incomplete, as indicated by `0x...` filenames in the playlist, but it can be updated independently as new information becomes available. The files are stored in the plugin directory next to `foo_unpack_vgm.dll`.

For *Cyberpunk 2077*, the list (`2077names.kark`) comes from the [WolvenKit repository](https://github.com/WolvenKit/WolvenKit/commits/main/WolvenKit.Common/Resources/usedhashes.kark). For best results, this file should be contemporary to the game's installation, as later releases may remove some content from the game and from the list.

## Acknowledgements

This project mostly consists of existing code and data from these sources, glued together:

[vgmstream](https://github.com/vgmstream/vgmstream)  
[SparkIV](https://code.google.com/archive/p/gtaivtools/)  
OpenIV  
[CodeWalker](https://github.com/Dexyfex/codewalker)  
[rpf-archive-rs](https://github.com/VIRUXE/rpf-archive-rs)  
[Chipicao](http://s1.zetaboards.com/Kottons_Chop_Shop/topic/4183888/1/)  
[Xarlith](https://turboduck.net/profile/15055-xarlith/)  
[TDU2.BIG.Tool](https://github.com/Ekey/TDU2.BIG.Tool)  
[tdumt2](https://github.com/djey47/tdumt2/)  
[red4lib](https://github.com/rfuzzo/red4lib)  
[WolvenKit](https://github.com/WolvenKit/WolvenKit/)  

*May contain AI-generated code.*
