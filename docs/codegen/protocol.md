This file goes into the generation of the protocols.

** Annotations **

*** enum ***

  * `flags` - generates a keycap_enum_flags

*** struct / message ***

  * `expected_size=<int>` - generates public data member `static constexpr size_t expected size = <int>`


*** attribute ***

  * `is_size` - Will generate a marker to rember the position in the encoder buffer. After all data has been added to the stream, the remebered value will be set to the streams size after the remembered position.
  * `endian_reverse` - Reverses the endianess of a primitive data type before it'll be written to / read from the stream.
  * `reverse` - Reverses the data. Assumes type has .begin() and .end() methods.
  * `zero_terminated` - May only be used with `string` type. String will not have a size byte prepended. Instead, a 0 byte will be appended.
  * `size_type="type"` - May only be used with `repeated` specifier. Sets the type of the size that'll be prepended before the repeatable data. If this annotation isn't used, uint8 is assumed.
  * `requires="condition"` - May only be used with `optional` specifier or `compressed` annotation. Overwrites the optional check with the given condition for `optional`. Condition will be generated as is. Will check size for `compressed`. Condition will be generated as size_{{attrib/name}} <= {{condition}}
  * `expects=value` - The value will be checked against the given value when decoded. Value may be in quotations for user defined types or strings.
  * `comment="text"` - Currently does nothing. Used for documentation within protocol definition files.
  * `compressed` - Data is zip compressed. Right now only works with repeated and exhausts the entire remaining stream.
  * `no_size` - May only be used with `repeated` specifier. Currently only implemented for encoding. No size will be prepended.