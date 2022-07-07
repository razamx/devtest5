Samples API Reference {#samples_api}
=========================================================

Samples APIs are an abstraction layer inside sample applications and demos to improve readability.
They are not intended to be used by Intel customers for product development.
There is no guarantee that these APIs will persist between releases.

### Real-Time Communication Demo API Reference #################################

The real-time communication demo uses the APIs listed in the following table:
<table>
  <caption id="tcc_rt_communication_demo_internal_api">Real-Time Communication Demo APIs</caption>
  <tr><th>API Name<th>Header File<th>Description</tr>

  <tr><td rowspan="4">Data Manager Helpers<td>[timer](@ref ptimer.h)<td>API for the real-time timer used in the sample</tr>
  <tr><td>[check](@ref pcheck.h)<td>Run-time check utilities used in the sample</tr>
  <tr><td>[config](@ref pconfig.h)<td>Application configuration manager used in the sample</tr>
  <tr><td>[Data source](@ref pdatasource.h)<td>API for reading and writing data for OPC UA* Server</tr>
  <tr><td rowspan="8">OPC UA* Server Helpers<td>[OPC UA* Publisher](@ref opcua_publish.h)<td>Helpers for working with OPC UA* Publishers</tr>
  <tr><td>[OPC UA* Subscriber](@ref opcua_subscribe.h)<td>Helpers for working with OPC UA* Subscribers</tr>
  <tr><td>[JSON](@ref json_helper.h)<td>API for parsing OPC UA* config settings in JSON format</tr>
  <tr><td>[OPC UA* Common](@ref opcua_common.h)<td>Helper API for working with OPC UA* Server</tr>
  <tr><td>[OPC UA* Shared](@ref opcua_shared.h)<td>Shared memory structures for OPC UA* Server IPC</tr>
  <tr><td>[OPC UA* Utils](@ref opcua_utils.h)<td>OPC UA* Utility functions</tr>
  <tr><td>[OPC UA* Custom](@ref opcua_custom.h)<td>Thread customization for OPC UA* Server</tr>
  <tr><td>[OPC UA* Datasource](@ref opcua_datasource.h)<td>Adaptation of the datasource interface for the OPC UA* Server</tr>
</table>
