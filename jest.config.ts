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
};
