API Reference {#index}
===

Intel® Time Coordinated Computing Tools (Intel® TCC Tools), consisting of
C language APIs, tools, and sample applications,
enable developers to analyze the behavior of real-time applications and access
certain Intel® TCC hardware features. For more documentation, see the
<a href="https://software.intel.com/content/www/us/en/develop/tools/time-coordinated-computing-tools.html">product page</a>.

## APIs

This reference covers public APIs and samples APIs.

* **Public APIs** are intended to be used by Intel customers for product development. Customers can expect these APIs to be well-defined and maintained.
* **Samples APIs** are *not* intended to be used by Intel customers for product development.
  These APIs are an abstraction layer inside sample applications and demos to improve readability.
  There is no guarantee that these APIs will persist between releases.


### Public APIs

The public API headers are grouped by topics:

| API Name    | Header File                         | Description                                                     |
| ------------|-------------------------------------|-----------------------------------------------------------------|
| Cache allocation library | [cache](@ref cache.h)      | Allocate memory buffers based on latency requirements.          |
| Measurement library | [measurement](@ref measurement.h)   | Measure latency and monitor deadline violations.                |
| Measurement helpers | [measurement_helpers](@ref measurement_helpers.h)   | Helper functions for the measurement library. |
| Error codes | [err_code](@ref err_code.h)         | Error enums and descriptions.                                   |

### Samples APIs

@subpage samples_api


## Legal Information

@subpage legal_information
