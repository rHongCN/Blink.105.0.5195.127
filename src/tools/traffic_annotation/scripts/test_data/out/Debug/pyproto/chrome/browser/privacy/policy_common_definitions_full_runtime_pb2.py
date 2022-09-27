# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: policy_common_definitions_full_runtime.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='policy_common_definitions_full_runtime.proto',
  package='enterprise_management',
  syntax='proto2',
  serialized_options=None,
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n,policy_common_definitions_full_runtime.proto\x12\x15\x65nterprise_management\"\x1d\n\nStringList\x12\x0f\n\x07\x65ntries\x18\x01 \x03(\t\"\x92\x01\n\rPolicyOptions\x12H\n\x04mode\x18\x01 \x01(\x0e\x32/.enterprise_management.PolicyOptions.PolicyMode:\tMANDATORY\"7\n\nPolicyMode\x12\r\n\tMANDATORY\x10\x00\x12\x0f\n\x0bRECOMMENDED\x10\x01\x12\t\n\x05UNSET\x10\x02\"a\n\x12\x42ooleanPolicyProto\x12<\n\x0epolicy_options\x18\x01 \x01(\x0b\x32$.enterprise_management.PolicyOptions\x12\r\n\x05value\x18\x02 \x01(\x08\"a\n\x12IntegerPolicyProto\x12<\n\x0epolicy_options\x18\x01 \x01(\x0b\x32$.enterprise_management.PolicyOptions\x12\r\n\x05value\x18\x02 \x01(\x03\"`\n\x11StringPolicyProto\x12<\n\x0epolicy_options\x18\x01 \x01(\x0b\x32$.enterprise_management.PolicyOptions\x12\r\n\x05value\x18\x02 \x01(\t\"\x87\x01\n\x15StringListPolicyProto\x12<\n\x0epolicy_options\x18\x01 \x01(\x0b\x32$.enterprise_management.PolicyOptions\x12\x30\n\x05value\x18\x02 \x01(\x0b\x32!.enterprise_management.StringList'
)



_POLICYOPTIONS_POLICYMODE = _descriptor.EnumDescriptor(
  name='PolicyMode',
  full_name='enterprise_management.PolicyOptions.PolicyMode',
  filename=None,
  file=DESCRIPTOR,
  create_key=_descriptor._internal_create_key,
  values=[
    _descriptor.EnumValueDescriptor(
      name='MANDATORY', index=0, number=0,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
    _descriptor.EnumValueDescriptor(
      name='RECOMMENDED', index=1, number=1,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
    _descriptor.EnumValueDescriptor(
      name='UNSET', index=2, number=2,
      serialized_options=None,
      type=None,
      create_key=_descriptor._internal_create_key),
  ],
  containing_type=None,
  serialized_options=None,
  serialized_start=194,
  serialized_end=249,
)
_sym_db.RegisterEnumDescriptor(_POLICYOPTIONS_POLICYMODE)


_STRINGLIST = _descriptor.Descriptor(
  name='StringList',
  full_name='enterprise_management.StringList',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='entries', full_name='enterprise_management.StringList.entries', index=0,
      number=1, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=71,
  serialized_end=100,
)


_POLICYOPTIONS = _descriptor.Descriptor(
  name='PolicyOptions',
  full_name='enterprise_management.PolicyOptions',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='mode', full_name='enterprise_management.PolicyOptions.mode', index=0,
      number=1, type=14, cpp_type=8, label=1,
      has_default_value=True, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
    _POLICYOPTIONS_POLICYMODE,
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=103,
  serialized_end=249,
)


_BOOLEANPOLICYPROTO = _descriptor.Descriptor(
  name='BooleanPolicyProto',
  full_name='enterprise_management.BooleanPolicyProto',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='policy_options', full_name='enterprise_management.BooleanPolicyProto.policy_options', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='value', full_name='enterprise_management.BooleanPolicyProto.value', index=1,
      number=2, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=251,
  serialized_end=348,
)


_INTEGERPOLICYPROTO = _descriptor.Descriptor(
  name='IntegerPolicyProto',
  full_name='enterprise_management.IntegerPolicyProto',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='policy_options', full_name='enterprise_management.IntegerPolicyProto.policy_options', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='value', full_name='enterprise_management.IntegerPolicyProto.value', index=1,
      number=2, type=3, cpp_type=2, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=350,
  serialized_end=447,
)


_STRINGPOLICYPROTO = _descriptor.Descriptor(
  name='StringPolicyProto',
  full_name='enterprise_management.StringPolicyProto',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='policy_options', full_name='enterprise_management.StringPolicyProto.policy_options', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='value', full_name='enterprise_management.StringPolicyProto.value', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=449,
  serialized_end=545,
)


_STRINGLISTPOLICYPROTO = _descriptor.Descriptor(
  name='StringListPolicyProto',
  full_name='enterprise_management.StringListPolicyProto',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='policy_options', full_name='enterprise_management.StringListPolicyProto.policy_options', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='value', full_name='enterprise_management.StringListPolicyProto.value', index=1,
      number=2, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=548,
  serialized_end=683,
)

_POLICYOPTIONS.fields_by_name['mode'].enum_type = _POLICYOPTIONS_POLICYMODE
_POLICYOPTIONS_POLICYMODE.containing_type = _POLICYOPTIONS
_BOOLEANPOLICYPROTO.fields_by_name['policy_options'].message_type = _POLICYOPTIONS
_INTEGERPOLICYPROTO.fields_by_name['policy_options'].message_type = _POLICYOPTIONS
_STRINGPOLICYPROTO.fields_by_name['policy_options'].message_type = _POLICYOPTIONS
_STRINGLISTPOLICYPROTO.fields_by_name['policy_options'].message_type = _POLICYOPTIONS
_STRINGLISTPOLICYPROTO.fields_by_name['value'].message_type = _STRINGLIST
DESCRIPTOR.message_types_by_name['StringList'] = _STRINGLIST
DESCRIPTOR.message_types_by_name['PolicyOptions'] = _POLICYOPTIONS
DESCRIPTOR.message_types_by_name['BooleanPolicyProto'] = _BOOLEANPOLICYPROTO
DESCRIPTOR.message_types_by_name['IntegerPolicyProto'] = _INTEGERPOLICYPROTO
DESCRIPTOR.message_types_by_name['StringPolicyProto'] = _STRINGPOLICYPROTO
DESCRIPTOR.message_types_by_name['StringListPolicyProto'] = _STRINGLISTPOLICYPROTO
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

StringList = _reflection.GeneratedProtocolMessageType('StringList', (_message.Message,), {
  'DESCRIPTOR' : _STRINGLIST,
  '__module__' : 'policy_common_definitions_full_runtime_pb2'
  # @@protoc_insertion_point(class_scope:enterprise_management.StringList)
  })
_sym_db.RegisterMessage(StringList)

PolicyOptions = _reflection.GeneratedProtocolMessageType('PolicyOptions', (_message.Message,), {
  'DESCRIPTOR' : _POLICYOPTIONS,
  '__module__' : 'policy_common_definitions_full_runtime_pb2'
  # @@protoc_insertion_point(class_scope:enterprise_management.PolicyOptions)
  })
_sym_db.RegisterMessage(PolicyOptions)

BooleanPolicyProto = _reflection.GeneratedProtocolMessageType('BooleanPolicyProto', (_message.Message,), {
  'DESCRIPTOR' : _BOOLEANPOLICYPROTO,
  '__module__' : 'policy_common_definitions_full_runtime_pb2'
  # @@protoc_insertion_point(class_scope:enterprise_management.BooleanPolicyProto)
  })
_sym_db.RegisterMessage(BooleanPolicyProto)

IntegerPolicyProto = _reflection.GeneratedProtocolMessageType('IntegerPolicyProto', (_message.Message,), {
  'DESCRIPTOR' : _INTEGERPOLICYPROTO,
  '__module__' : 'policy_common_definitions_full_runtime_pb2'
  # @@protoc_insertion_point(class_scope:enterprise_management.IntegerPolicyProto)
  })
_sym_db.RegisterMessage(IntegerPolicyProto)

StringPolicyProto = _reflection.GeneratedProtocolMessageType('StringPolicyProto', (_message.Message,), {
  'DESCRIPTOR' : _STRINGPOLICYPROTO,
  '__module__' : 'policy_common_definitions_full_runtime_pb2'
  # @@protoc_insertion_point(class_scope:enterprise_management.StringPolicyProto)
  })
_sym_db.RegisterMessage(StringPolicyProto)

StringListPolicyProto = _reflection.GeneratedProtocolMessageType('StringListPolicyProto', (_message.Message,), {
  'DESCRIPTOR' : _STRINGLISTPOLICYPROTO,
  '__module__' : 'policy_common_definitions_full_runtime_pb2'
  # @@protoc_insertion_point(class_scope:enterprise_management.StringListPolicyProto)
  })
_sym_db.RegisterMessage(StringListPolicyProto)


# @@protoc_insertion_point(module_scope)