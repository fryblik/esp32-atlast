// jQuery document

import {CodeJar} from '/www/codejar.min.js';
import {withLineNumbers} from '/www/linenumbers.min.js';

let ws;				// WebSocket connection
let pendingFile;	// File pending for upload

// WebSocket connection and handling
function connectWS() {
	ws = new WebSocket('ws://' + location.host + '/ws', ['arduino']);

	// Handle incoming JSON
	ws.onmessage = function(event) {
		// Parse received JSON
		let obj = JSON.parse(event.data);

		// TODO: DEBUG
		console.log(obj);

		// Decide message type
		if (obj.type == 'cli') {
			// Print message to CLI
			updateCliOut(obj.data);

		} else if (obj.type == 'fileList') {
			// Update file select
			updateFileList(obj.paths);

		} else if (obj.type == 'upload') {
			// Complete file upload
			if (obj.status == 'ready') {
				// ESP is ready for upload. Engage!
				uploadFile();
			} else {
				// ESP refused upload. Clear pending file object.
				pendingFile = null;
				alert('Upload aborted: ' + obj.status);
			}

		} else if (obj.type == 'delete') {
			// File deletion status
			if (obj.status == 'ok') {
				// File deleted, update file list
				requestFileList();
			} else {
				// TODO
				console.log('Deletion failed!');
			}
		}
	}
}

// CLI: on ENTER key send input
function sendInput() {
	const cliIn = $( '#cliIn' );
	cliIn.keypress(function() {
		let keycode = (event.keyCode ? event.keyCode : event.which);
		// ENTER key
		if (keycode == '13') {
			// Take input
			let msg = cliIn.val();

			// Empty input crashes ATLAST
			// TODO: send " " on empty message?
			if (!msg) {
				return;
			}

			// Wrap message in JSON and send
			let obj = {
				type: "cli",
				data: msg
			};
			ws.send(JSON.stringify(obj));

			// Clear input
			cliIn.val('');
		}
	});
}
	
// Add message to history
function updateCliOut(addition) {
	let cliOut = $( '#cliOut' );

	// Split message into lines, filter out empty strings
	let lines = addition.split('\n').filter(s => s);

	// Show lines separated with <br>
	$.each (lines, function(i, line) {
		if (!cliOut.is(':empty')) {
			cliOut.append('<br>');
		}
		cliOut.append(line);
	});

	// Scroll to bottom
	cliOut.scrollTop(Number.MAX_SAFE_INTEGER);
}

// Handler: On button click, kill running ATLAST program
function killProgram() {
	// Send KILL request JSON, include parameter from checkbox
	$( '#killButton' ).click(function() {
		if ($( '#restartTaskBox ').is(':checked')) {
			ws.send('{"type":"kill","restartTask":1}');
		} else {
			ws.send('{"type":"kill","restartTask":0}');
		}
	});
}

// Get valid file path from input field
// Return path if valid, null otherwise
function getFilePathIn() {
	// Input field
	let filePathIn = $( '#filePathIn' );
	let path = filePathIn.val();

	// Check for missing/invalid file path and notify user on violation
	if (!path) {
		alert('Please specify a save path.');
		filePathIn.focus();
		return null;
	} else if (path.length < 2 || path.charAt(0) != '/' || path.charAt(path.length - 1) == '/') {
		alert('Please specify a valid save path.');
		filePathIn.focus();
		return null;
	} else if (path.length > 31) {
		alert('File path can be at most 31 characters long. Current length: ' + path.length);
		filePathIn.focus();
		return null;
	}
	
	// File path is valid
	return path;
}


// Handler: On click let user select local file, and request upload
function requestFileUpload() {
	const fileUpButton = $( '#fileUpButton' );
	const fileUpDialog = $( '#fileUpDialog' );

	// Activate file selection dialog on button click
	fileUpButton.click(function() {
		fileUpDialog.click();
	});

	// Ask to upload file when selected
	fileUpDialog.change(function() {
		// Get valid save path from input field
		let path = getFilePathIn();

		// Request upload, if we got the path
		if (path) {
			pendingFile = fileUpDialog[0].files[0];

			// Send upload request in JSON
			let obj = {
				type: 'upload',
				name: path,
				size: pendingFile.size
			};
			ws.send(JSON.stringify(obj));
		}

		// Clear file dialog value (so same filename can trigger this event)
		fileUpDialog.val(null);
	});
}

// Upload file
function uploadFile() {
	// Send file blob
	ws.send(pendingFile);
	console.log('File sent.');

	// Clear pending file object
	pendingFile = null;

	// Request updated file list
	requestFileList();
}

// Handler: On click download and save file from ESP
function downloadFile() {
	// On download button click
	$( '#fileDnButton' ).click(function() {
		// Selected file option
		const selection = $( '#fileSelect option:selected' );

		// Ignore disabled option
		if (selection.prop('disabled')) {
			return;
		}

		// Get file path
		let path = selection.text();
		// Remove leading slash from path to give FileSaver a valid filename
		let filename = path.substring(1);
		// Download (FileSaver.js)
		saveAs(path, filename);
	});
}

// Handler: On click delete file from ESP
function deleteFile() {
	// On delete button click
	$( '#fileDelButton' ).click(function() {
		// Selected file option
		const selection = $( '#fileSelect option:selected' );

		// Ignore disabled option
		if (selection.prop('disabled')) {
			return;
		}

		// Send delete request with file path in JSON
		let obj = {
			type: 'delete',
			path: selection.text()
		};
		ws.send(JSON.stringify(obj));
	});
}

// Request file list
function requestFileList() {
	// Send file list request JSON
	ws.send('{"type":"fileList"}');
}

// Update file select from received array
function updateFileList(paths) {
	// Clear old options
	$( '#fileSelect' ).empty();

	// Add option for each array member
	$.each(paths, function(index, path) {   
		$( '#fileSelect' )
			.append($( '<option></option>' )
					   .text(path));
   });
}


/////////////////////////// Code editor ////////////////////////////

let jar;			// CodeJar object

// Create code editor
function initEditor() {
	// Highlighter
	const highlight = editor => {
		CodeMirror.runMode(editor.textContent, "forth", editor);
	}
	// Non-jQuery selector for CodeJar compatibility
	let editor = document.querySelector(".editor");
	// Turn tabs into spaces
	let opts = {tab: ' '.repeat(4)};

	// Initialize
	jar = new CodeJar(
		editor,
		withLineNumbers(highlight),
		opts);

	// TODO: actions on code update?
	// debug only, remove:
	jar.onUpdate(updatedCode => { console.log(updatedCode) });

	// Update contents
	let newCode = 'sample code';
	jar.updateCode(newCode);
}

// Handler: On click select and load local file into editor
function editLocalFile() {
	const editLocalButton = $( '#editLocalButton' );
	const editLocalDialog = $( '#editLocalDialog' );

	// Activate file selection dialog on button click
	editLocalButton.click(function() {
		editLocalDialog.click();
	});

	// Load file into editor when selected
	editLocalDialog.change(function() {
		let fileBlob = editLocalDialog[0].files[0];

		// Update file path field with suggested file path
		$( '#filePathIn' ).val('/atl/' + fileBlob.name);

		// Read file as text and update editor contents
		let reader = new FileReader();
		reader.onload = function() {
			jar.updateCode(reader.result);
		}
		reader.readAsText(fileBlob);

		// Clear file dialog value (so we can load with same filename repeatedly)
		editLocalDialog.val(null);
	});
}

// Handler: On click create file object from editor content and request upload
function requestCodeUpload() {
	// Ask to upload file on button click
	$( '#editorUpButton' ).click(function() {
		// Get valid save path from input field
		let path = getFilePathIn();
		// Check if we got the path
		if (!path) {
			return;
		}

		// Get editor content
		let code = jar.toString();

		// Create file object with editor content
		pendingFile = new Blob(
			[code],
			{type: "text/plain;charset=utf-8"}
		);

		// Send upload request in JSON
		let obj = {
			type: 'upload',
			name: path,
			size: code.length
		};
		ws.send(JSON.stringify(obj));
	});
}

// Handler: Edit selected file from device storage
function editDeviceFile() {
	$( '#fileEditButton' ).click(function() {
		// Selected file option
		const selection = $( '#fileSelect option:selected' );
		// Ignore disabled option
		if (selection.prop('disabled')) {
			return;
		}
		// Get file path
		let path = selection.text();
		// Update file path field with loaded file path
		$( '#filePathIn' ).val(path);
		// Fetch data from file's URL and put it in the editor
		$.get(path, function(data) {
			jar.updateCode(data);
		});
	});
}

/////////////// Run these handlers when DOM is ready ///////////////

$(initEditor);
$(connectWS);
$(killProgram);
$(sendInput);
$(requestFileUpload);
$(downloadFile);
$(deleteFile);
$(editLocalFile);
$(requestCodeUpload);
$(editDeviceFile);