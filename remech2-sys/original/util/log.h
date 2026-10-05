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
	// Logs p_text at the debug level
	void MechLogDebug(const char* p_text);

#ifdef __cplusplus
}
#endif

#endif // LOG_H
