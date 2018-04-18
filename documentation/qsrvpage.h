/**
@page qsrv_page QSRV

@section qsrv_config QSRV Configuration

By default QSRV exposes all Process Variables (fields of process database records).
In addition to these "single" PVs are special "group" PVs.

@subsection qsrv_single Single PVs

"single" PVs are the same set of names server by the Channel Access server (RSRV).
This is all accessible record fields.
So all data which is accessible via Channel Access is also accessible via PVAccess.

QSRV presents all "single" PVs as Structures conforming to the
Normative Types NTScalar, NTScalarArray, or NTEnum depending on the native DBF field type.

@subsection qsrv_group_def Group PV definitions

A group is defined using a JSON syntax.
Groups are defined with respect to a Group Name,
which is also the PV name.
So unlike records, the "field" of a group have a different meaning.
Group field names are _not_ part of the PV name.

A group definition is split among several records.
For example of a group including two records is:

@code
record(ai, "rec:X") {
  info(Q:group, {
    "grp:name": {
        "X": {+channel:"VAL"}
    }
  })
}
record(ai, "rec:Y") {
  info(Q:group, {
    "grp:name": {
        "Y": {+channel:"VAL"}
    }
  })
}
@endcode

This group, named "grp:name", has two fields "X" and "Y".

@code
$ pvget grp:name
grp:name
structure 
    epics:nt/NTScalar:1.0 X
        double value 0
        alarm_t alarm INVALID DRIVER UDF
        time_t timeStamp <undefined> 0
...
    epics:nt/NTScalar:1.0 Y
        double value 0
        alarm_t alarm INVALID DRIVER UDF
        time_t timeStamp <undefined> 0
...
@endcode

@subsection qsrv_group_ref Group PV reference

@code
record(...) {
    info(Q:group, {
        "<group_name>":{
            +id:"some/NT:1.0",  # top level ID
            +meta:"FLD",        # map top level alarm/timeStamp
            +atomic:true,       # whether monitors default to multi-locking atomicity
            "<field.name>":{
                +type:"scalar", # controls how map VAL mapped onto <field.name>
                +channel:"VAL",
                +id:"some/NT:1.0",
                +trigger:"*",   # "*" or comma seperated list of <field.name>s
                +putorder:0,    # set for fields where put is allowed, processing done in increasing order
            }
        }
    })
}
@endcode

@subsubsection qsrv_group_map_types Field mapping types

@li "scalar" or ""
@li "plain"
@li "any"
@li "meta"
@li "proc"

The "scalar" mapping places an NTScalar or NTScalarArray as a sub-structure.

The "plain" mapping ignores all meta-data and places only the "value" as a field.
The "value" is equivalent to '.value' of the equivalent NTScalar/NTScalarArray as a field.

The "any" mapping places a variant union into which the "value" is placed.

The "meta" mapping ignores the "value" and places only the alarm and time
meta-data as sub-fields.
The special group level tag 'meta:""' allows these meta-data fields to be
placed in the top-level structure.

The "proc" mapping uses neither "value" nor meta-data.
Instead the target record is processed during a put.

@subsubsection qsrv_group_map_trig Field Update Triggers

The field triggers define how changes to the consitutent field
are translated into a subscription update to the group.

The most use of these are "" which means that changes to the field
are ignored, and do not result group update.
And "*" which results in a group update containing the most recent
values/meta-data of all fields.

It may be useful to specify a comma seperated list of field names
so that changes may partially update the group.

@subsection qsrv_stamp QSRV Timestamp Options

QSRV has the ability to perform certain transformations on the timestamp before transporting it.
The mechanism for configuring this is the "Q:time:tag" info() tag.

@subsubsection qsrv_stamp_nslsb Nano-seconds least significant bits

Setting "Q:time:tag" to a value of "nsec:lsb:#", where # is a number between 0 and 32,
will split the nanoseconds value stored in the associated record.
The least significant # bits are stored in the 'timeStamp.userTag' field.
While the remaining 32-# bits are stored in 'timeStamp.nanoseconds' (without shifting).

For example, in the following situation 16 bits are split off.
If the nanoseconds part of the record timestamp is 0x12345678,
then the PVD structure would include "timeStamp.nanoseconds=0x12300000"
and "timeStamp.userTag=0x45678".

@code
record(ai, "...") {
  info(Q:time:tag, "nsec:lsb:20")
}
@endcode

@subsection qsrv_link PVAccess Links

When built against Base >= 3.16.1, support is enabled for PVAccess links,
which are analogous to Channel Access (CA) links.  However, the syntax
for PVA links is quite different.

@warning The PVA Link syntax shown below is provisional and subject to change.

A simple configuration using defaults is

@code
record(longin, "tgt") {}
record(longin, "src") {
    field(INP, {pva:"tgt"})
}
@endcode

This is a shorthand for

@code
record(longin, "tgt") {}
record(longin, "src") {
    field(INP, {pva:{pv:"tgt"}})
}
@endcode

Some additional keys (beyond "pv") may be used.
Defaults are shown below:

@code
record(longin, "tgt") {}
record(longin, "src") {
    field(INP, {pva:{
        pv:"tgt",
        field:"",   # may be a sub-field
        Q:4,        # monitor queue depth
        proc:none,  # Request record processing (side-effects).
        sevr:false, # Maximize severity.
        monorder:0, # Order of record processing as a result of CP and CPP
        defer:false # Defer put
    }})
}
@endcode

@subsubsection qsrv_link_pv pv: Target PV name

The PV name to search for.
This is the same name which could be used with 'pvget' or other client tools.

@subsubsection qsrv_link_field field: Structure field name

The name of a sub-field of the remotely provided Structure.
By default, an empty string "" uses the top-level Structure.

If the top level structure, or a sub-structure is selected, then
it is expeccted to conform to NTScalar, NTScalarArray, or NTEnum
to extract value and meta-data.

If the sub-field is an PVScalar or PVScalarArray, then a value
will be taken from it, but not meta-data will be available.

@todo Ability to traverse through unions and into structure arrays (as with group mappings).

@subsubsection qsrv_link_Q Q: Monitor queue depth

Requests a certain monitor queue depth.
The server may, or may not, take this into consideration when selecting
a queue depth.

@subsubsection qsrv_link_pipeline pipeline: Monitor flow control

Expect that the server supports PVA monitor flow control.
If not, then the subscription will stall (ick.)

@subsubsection qsrv_link_proc proc: Request record processing (side-effects)

The meaning of this option depends on the direction of the link.

For output links, this option allows a request for remote processing (side-effects).

@li none (default) - Make no special request.  Uses a server specific default.
@li false, "NPP" - Request to skip processing.
@li true, "PP" - Request to force processing.
@li "CP", "CPP" - For output links, an alias for "PP".

For input links, this option controls whether the record containing
the PVA link will be processed when subscription events are received.

@li none (default), false, "NPP" - Do not process on subscription updates.
@li true, "CP" - Always process on subscription updates.
@li "PP", "CPP" - Process on subscription updates if SCAN=Passive

@subsubsection qsrv_link_sevr sevr: Alarm propagation

This option controls whether reading a value from an input PVA link
has the addition effect of propagating any alarm via the Maximize
Severity process.

@warning Not yet implemented

@li false - Do not maximize severity.
@li true - Maximize alarm severity
@li "MSI" - Maximize only if the remote severity is INVALID.

@subsubsection qsrv_link_monorder monorder: Monitor processing order

When multiple record target the same target PV, and request processing
on subscription updates.  This option allows the order of processing
to be specified.

Record are processed in increasing order.
monorder=-1 is processed before monorder=0.
Both are processed before monorder=1.

@subsubsection qsrv_link_defer defer: Defer put

By default (defer=false) an output link will immediately
start a PVA Put operation.  defer=true will store the
new value in an internal cache, but not start a PVA Put.

This option, in combination with field: allows a single
Put to contain updates to multiple sub-fields.

*/
