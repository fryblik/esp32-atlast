// jQuery document

// Initiate WebSocket connection
let ws = new WebSocket('ws://' + location.host + '/ws', ['arduino']);

// File pending for upload
let pendingFile;

// Handle incoming JSON
ws.onmessage = function(event) {
	// Parse received JSON
	let obj = JSON.parse(event.data);

	// DEBUG
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

	// Show lines separated with <br> (filter out empty strings in split)
	lines = addition.split('\n').filter(s => s);
	$.each (lines, function(i, line) {
		cliOut.append('<br>');
		cliOut.append(line);
	});

	// Scroll to bottom
	cliOut.scrollTop(Number.MAX_SAFE_INTEGER);
}

// Initiate file upload: click to select file and request upload
function initUpload() {
	const fileUpButton = $( '#fileUpButton' );
	const fileUpDialog = $( '#fileUpDialog' );

	// Activate file selection dialog on button click
	fileUpButton.click(function() {
		fileUpDialog.click();
	});

	// Ask to upload file when selected
	fileUpDialog.change(function() {
		pendingFile = fileUpDialog[0].files[0];

		// Send upload request in JSON
		let obj = {
			type: 'upload',
			name: pendingFile.name,
			size: pendingFile.size
		};
		ws.send(JSON.stringify(obj));

		// Clear file dialog value (so we can send with same filename repeatedly)
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

// Download and save file from ESP
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
		// Remove leading slash from path to get filename
		let filename = path.substring(1);
		// Download
		saveAs(path, filename);
	});
}

// Delete file from ESP
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

// Run these handlers when DOM is ready
$(sendInput);
$(initUpload);
$(downloadFile);
$(deleteFile);