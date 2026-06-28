/*
 * For a detailed explanation regarding each configuration property and type check, visit:
 * https://jestjs.io/docs/configuration
 */

export default {
  collectCoverage: true,
  coverageDirectory: "coverage",
  coverageProvider: "v8",
  preset: 'ts-jest',
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
