# TODO
* [] Thread allocators are not getting destroyed. Investigate.
* [] Find out safer way to provide VM index from the lua state.
* []

# Done
* [x] Review file loading to memory functions. Implement: load to buffer, ~load given allocator~
* [x] Implement do_file_at with more granularity
* [x] Implement safety net in lua_log_message with try catch.
* [x] Implement more log functions.
* [X] Find out the VM index from the lua state.