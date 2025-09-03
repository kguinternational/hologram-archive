const fs = require('fs');
const path = require('path');

// Load the WASM module
require('./test-conservation.js')({
    print: function(text) { console.log(text); },
    printErr: function(text) { console.error(text); }
}).then(function(Module) {
    console.log('Running Atlas Conservation WASM tests in Node.js...');
    try {
        let result = Module._main();
        console.log('\nTest completed with exit code:', result);
        process.exit(result);
    } catch (e) {
        console.error('Test execution failed:', e);
        process.exit(1);
    }
}).catch(function(error) {
    console.error('Failed to load WASM module:', error);
    process.exit(1);
});