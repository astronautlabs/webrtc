/* eslint no-process-exit:0 */
import binding from '../../../binding';
const result = binding.test();
process.exit(result);