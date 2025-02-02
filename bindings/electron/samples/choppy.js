if(process.platform==='linux') {
    Choppy=require('../toolbit-lib/lib/toolbit-linux.node').Choppy;   
} else if(process.platform==='darwin') {
    Choppy=require('../toolbit-lib/lib/toolbit-macos.node').Choppy;   
} else if(process.platform==='win32') {
    Choppy=require('../toolbit-lib/lib/toolbit-mswin.node').Choppy;   
}

let choppy=new Choppy();
choppy.open();

console.log('voltage:' + choppy.getVoltage());
console.log('current:' + choppy.getCurrent());
