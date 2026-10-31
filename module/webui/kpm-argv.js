// Preserve each line verbatim. No shell tokenization of individual KPM args.
export function splitKpmArgvLines(text) {
    if (typeof text !== 'string' || text.length === 0) return [];
    return text.replace(/\r\n/g, '\n').split('\n');
}

export function validateKpmArgv(args) {
    if (!Array.isArray(args) || args.length > 32) return false;
    let wireBytes = 'PNARGV1:'.length + String(args.length).length + 1;
    for (const arg of args) {
        if (typeof arg !== 'string' || arg.includes('\0')) return false;
        const len = new TextEncoder().encode(arg).length;
        wireBytes += String(len).length + 1 + len;
        if (wireBytes > 1023) return false;
    }
    return true;
}

export function buildKpmCommand(operation, target, value, quote) {
    if (operation !== 'load' && operation !== 'control') throw new Error('Invalid KPM operation');
    if (!target || typeof quote !== 'function') throw new Error('Invalid KPM target');
    const encoded = Array.isArray(value);
    if (encoded && !validateKpmArgv(value)) throw new Error('KPM arguments exceed 32 items or 1023 bytes');
    const verb = operation === 'load' ? (encoded ? 'loadv' : 'load') : (encoded ? 'ctlv' : 'ctl0');
    let command = 'kpatch kpm ' + verb + ' ' + quote(target);
    if (encoded) {
        for (const arg of value) command += ' ' + quote(arg);
    } else if (operation === 'control') {
        command += ' ' + quote(value);
    }
    return command;
}
