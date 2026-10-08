#ifndef LOG_H
#define LOG_H

// Logging, implemented on the Rust side (src/log.rs). It replaces MessageBox and
// OutputDebugString.

#ifdef __cplusplus
extern "C"
{
#endif

	// Logs p_text as an error
	void MechLogError(const char* p_text);
	// Same as MechLogError except it formats the input
	void MechLogErrorf(const char* p_format, ...);
	// Logs p_text at the debug level
	void MechLogDebug(const char* p_text);
	// Same as MechLogDebug except it formats the input
	void MechLogDebugf(const char* p_format, ...);
	// Logs p_text at the trace level
	void MechLogTrace(const char* p_text);
	// Same as MechLogTrace except it formats the input
	void MechLogTracef(const char* p_format, ...);

#ifdef __cplusplus
}
#endif

#endif // LOG_H
