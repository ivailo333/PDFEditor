# PDFEditor

Clean Qt Creator project for a C++/Qt PDF reader and editor.

Open this file in Qt Creator:

```text
PDFEditor.pro
```

## Dependencies

Required for the GUI:

- Qt 6
- Qt Widgets

Required for PDF viewing:

- Qt PDF module

Required for real PDF saving/editing:

- QPDF development package

The project is intentionally tolerant: it still opens in Qt Creator if Qt PDF or QPDF are missing. Missing features show clear warnings instead of breaking project loading.

## Enable Qt PDF

If Qt Creator shows:

```text
Unknown module(s) in QT: pdf
```

install Qt PDF from Qt Maintenance Tool for the same Qt Kit.

## Enable QPDF on Windows

Install QPDF, then set `QPDF_ROOT` in Qt Creator:

```text
Projects -> Build Environment -> Add
QPDF_ROOT=C:/vcpkg/installed/x64-mingw-dynamic
```

For MSVC kits use the matching triplet, for example:

```text
QPDF_ROOT=C:/vcpkg/installed/x64-windows
```

`QPDF_ROOT` must point to the folder that contains:

```text
include/qpdf/QPDF.hh
lib/
```

After changing dependencies:

```text
Build -> Run qmake
Build -> Rebuild Project
```

## Features

- Open one PDF document
- Create a new blank PDF document
- Add blank pages to a new PDF document
- Read pages with next/previous controls
- Zoom in, zoom out, and reset zoom
- Create editable text boxes by clicking on the current page
- Type directly in the text box
- Choose font family and font size
- Use bold, italic, and underline text styles
- Double-click an existing text box to edit it
- Drag an existing text box to move it
- Press Delete to remove the selected text box
- Save a new PDF with QPDF `/FreeText` annotations when QPDF is configured
