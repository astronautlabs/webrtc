import { fromRollup } from '@web/dev-server-rollup';
import { esbuildPlugin } from '@web/dev-server-esbuild';
import rollupCommonJS from '@rollup/plugin-commonjs';
import rollupNodeResolve from '@rollup/plugin-node-resolve';
import rollupNodePolyfills from 'rollup-plugin-node-polyfills';

const commonjs = fromRollup(rollupCommonJS);
const nodeResolve = fromRollup(rollupNodeResolve);
const nodePolyfills = fromRollup(rollupNodePolyfills);
export default {
    nodeResolve: true,
    plugins: [
        nodePolyfills(),
        nodeResolve({
            preferBuiltins: false
        }),
        commonjs({
            include: [
                './node_modules/**/*'
            ],
            strictRequires: true
        }),
        esbuildPlugin({ 
            ts: true, 
            //tsconfig: './tsconfig.esm.json',
            target: 'auto'
        })
    ],
};