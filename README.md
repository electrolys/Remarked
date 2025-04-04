## Remarked
[![rm2](https://img.shields.io/badge/rM2-supported-green)](https://remarkable.com/store/remarkable-2)
[![Discord](https://img.shields.io/discord/385916768696139794.svg?label=reMarkable&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/ATqQGfu)

The open source alternative to writing in xochitl. 
It is not the most feature filled but it is quite useful.

### Features
- The Select tool is currently not implemented and the UI is pretty bare bones.
- You can Cut a page with X and Paste it with V there is a buffer of pages Cut in the document Copies.
- You make links to documents with + and remove them with - it goes back to the previous tool after each of said operations but that isn't reflected in the UI currently.
- Links with the same names go to the same document.
- All the documents are stored in a sqlite database at /home/root/notes.db.
- Clicking the File/Page indicator returns you to the Home page.
- Swiping left or right switches pages and up or down scrolls

### Building
Simply source the Remarkable enviroment and call make. The binary will be in bin/ and the draft file is remarked.draft
