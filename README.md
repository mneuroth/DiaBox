# DiaBox
An app to backup and label images.

## Github Actions Status

Platforms:
    * Linux: [![Linux Desktop Build](https://github.com/mneuroth/DiaBox/actions/workflows/linux.yml/badge.svg)](https://github.com/mneuroth/DiaBox/actions/workflows/linux.yml)
	* MacOS: [![MacOS Desktop Build](https://github.com/mneuroth/DiaBox/actions/workflows/macos.yml/badge.svg)](https://github.com/mneuroth/DiaBox/actions/workflows/macos.yml)
    * Windows: [![Windows Desktop Build](https://github.com/mneuroth/DiaBox/actions/workflows/windows.yml/badge.svg)](https://github.com/mneuroth/DiaBox/actions/workflows/windows.yml)
    * WASM: [![Qt WASM Build (jurplel)](https://github.com/mneuroth/DiaBox/actions/workflows/wasm_jurplel.yml/badge.svg)](https://github.com/mneuroth/DiaBox/actions/workflows/wasm_jurplel.yml)

## WASM

Download the `dia_box_wasm.zip` file and extract the content. Open a shell and change to the extract directory. Run a HTTP server with this command:

```
>python -m http.server 8080
```

Open a Web-Browser and enter this url: `localhost:8080/dia_box.html`
