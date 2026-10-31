import { describe, it, expect } from 'vitest';
import { splitKpmArgvLines, validateKpmArgv, buildKpmCommand } from '../kpm-argv.js';

const quote = (v) => "'" + v.replaceAll("'", "'\\''") + "'";

describe('KPM opt-in argv UI', () => {
    it('preserves whitespace, empty args, unicode and quotes', () => {
        const args = splitKpmArgvLines("mode=fast\ntwo words\n\n雪\na'b");
        expect(args).toEqual(['mode=fast','two words','','雪',"a'b"]);
        expect(validateKpmArgv(args)).toBe(true);
        expect(buildKpmCommand('load','/tmp/a.kpm',args,quote)).toContain("loadv '/tmp/a.kpm'");
    });
    it('never changes legacy invocation by default', () => {
        expect(buildKpmCommand('load','/tmp/a.kpm',null,quote)).toBe("kpatch kpm load '/tmp/a.kpm'");
        expect(buildKpmCommand('control','demo','action=restart',quote))
            .toBe("kpatch kpm ctl0 'demo' 'action=restart'");
    });
    it('enforces exact UTF8 transport limit and count', () => {
        expect(validateKpmArgv(Array(33).fill(''))).toBe(false);
        expect(validateKpmArgv(['x'.repeat(1020)])).toBe(false);
        expect(validateKpmArgv(['\0'])).toBe(false);
        expect(() => buildKpmCommand('load','demo',['x'.repeat(1020)],quote)).toThrow();
        expect(splitKpmArgvLines('')).toEqual([]);
        expect(buildKpmCommand('control','demo',[],quote)).toBe("kpatch kpm ctlv 'demo'");
    });
});
