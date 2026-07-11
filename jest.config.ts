/*
 * For a detailed explanation regarding each configuration property and type check, visit:
 * https://jestjs.io/docs/configuration
 */

import type { Config } from 'jest';
import { createDefaultPreset } from 'ts-jest';

const config: Config = {
  collectCoverage: true,
  coverageDirectory: "coverage",
  coverageProvider: "v8",
  ...createDefaultPreset({
    diagnostics: {
        ignoreCodes: [151002] // TS151002: Using hybrid module kind (Node16/18/Next) is only supported in "isolatedModules: true"
    }
  }),
  roots: [
    "lib/"
  ],
  testMatch: [ "**/*.test.ts" ],
  watchman: false,

  // Our tests can be quite long.
  testTimeout: 120_000,

  // We currently need some more time after cleaning up RTC objects, so we relax Jest's exit timeout to account for this.
  // TODO(liam): I still think we can do better than this.
  openHandlesTimeout: 5000,
};

export default config;