#ifndef ERRORFLAGS_H
#define ERRORFLAGS_H

namespace THR
{
#define ERROR_FLAGS_SUCCESS(status) ((status) == ErrorFlags::None)
#define ERROR_FLAGS_FAILURE(status) ((status) != ErrorFlags::None)

	enum ErrorFlags
	{
		/// <summary>
		/// Indicates no errors; proper operation.
		/// </summary>
		None = 0,
		/// <summary>
		/// Provided parameter is out of range or null.
		/// </summary>
		InvalidParameter = 1,
		/// <summary>
		/// Operation as requested is not supported, or is unimplemented.
		/// </summary>
		OperationUnsupported = 2,
		/// <summary>
		/// Buffer underflow.
		/// </summary>
		Underflow = 4,
		/// <summary>
		/// Buffer overflow.
		/// </summary>
		Overflow = 8,
		/// <summary>
		/// An external IO resource is unable to be used.
		/// </summary>
		ResourceUnavailable = 16,
		/// <summary>
		/// Caller does not have authority to perform an operation.
		/// </summary>
		PermissionDenied = 32,
		/// <summary>
		/// Resource is not responding to queries.
		/// </summary>
		NotResponding = 64,
		/// <summary>
		/// Class/resource/dependency is not properly initialized first.
		/// </summary>
		NotInitialized = 128,
		/// <summary>
		/// Operation did not succeed.
		/// </summary>
		Unsuccessful = 256,
		/// <summary>
		/// Operation/process already running.
		/// </summary>
		AlreadyRunning = 512,
		/// <summary>
		/// Resource is disposed; cannot be used further.
		/// </summary>
		Disposed = 1024,
		/// <summary>
		/// Message frame isn't correct.
		/// </summary>
		FramingError = 2048,
		/// <summary>
		/// Checksum failure.
		/// </summary>
		ChecksumFailure = 4096,
		/// <summary>
		/// Action cannot be performed because the object is not in the correct state to allow for it.
		/// </summary>
		InvalidState = 8192
	};
}

#endif