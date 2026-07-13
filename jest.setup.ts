// Add this at the very top of your test or main file
process.report.reportOnFatalError = true;
process.report.reportOnUncaughtException = true;
// Optional: trigger it on a specific signal
process.report.reportOnSignal = true; 
process.report.signal = 'SIGSEGV';