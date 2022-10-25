#!/usr/bin/perl
# SPDX-License-Identifier: GPL-2.0-only */
# Copyright 2022 Collabora Ltd.

sub convert_type_to_json_type {
	my $type = shift;
	if ($type eq __u8 || $type eq __u16 || $type eq __s8 || $type eq __s16 || $type eq __s32) {
		return "int";
	}
	if ($type eq __u32) {
		return "int64";
	}
	if ($type eq __u64) {
		return "uint64";
	}
	if ($type eq struct) {
		return;
	}
	print "v4l2_tracer: error: couldn't convert $type to json_object type.\n";
	return;
}

sub get_index_letter {
	my $index = shift;
	if ($index eq 0) {return "i";}
	if ($index eq 1) {return "j";}
	if ($index eq 2) {return "k";}
	if ($index eq 3) {return "l";}
	if ($index eq 4) {return "m";}
	if ($index eq 5) {return "n";}
	if ($index eq 6) {return "o";} # "p" is saved for pointer
	if ($index eq 8) {return "q";}
	return "z";
}

$flag_func_name;

sub flag_gen {
	my $flag_type = shift;

	if ($flag_type =~ /fwht/) {
		$flag_func_name = v4l2_ctrl_fwht_params_;
	} elsif ($flag_type =~ /vp8_loop_filter/) {
		$flag_func_name = v4l2_vp8_loop_filter_;
	} else {
		($flag_func_name) = ($_) =~ /#define (\w+_)FL.+/;
		$flag_func_name = lc $flag_func_name;
	}

	printf $fh_common_info_h "constexpr flag_def %sflag_def[] = {\n", $flag_func_name;

	($flag) = ($_) =~ /#define\s+(\w+)\s+.+/;
	printf $fh_common_info_h "\t{ $flag, \"$flag\" },\n"; #get the first flag

	while (<>) {
		next if ($_ =~ /^\/?\s?\*.*/); #skip comments between flags if any
		next if $_ =~ /^\s*$/; #skip blank lines between flags if any
		last if ((grep {!/^#define\s+\w+_FL/} $_) && (grep {!/^#define V4L2_VP8_LF/} $_));
		($flag) = ($_) =~ /#\s*define\s+(\w+)\s+.+/;

		# don't include flags that are masks
		next if ($flag_func_name eq v4l2_buf_) && ($flag =~ /.*TIMESTAMP.*/ || $flag =~ /.*TSTAMP.*/);
		next if ($flag_func_name eq v4l2_ctrl_fwht_params_) && ($flag =~ /.*COMPONENTS.*/ || $flag =~ /.*PIXENC.*/);
		next if ($flag =~ /.*MEDIA_LNK_FL_LINK_TYPE.*/);
		next if ($flag =~ /.*MEDIA_ENT_ID_FLAG_NEXT.*/);

		printf $fh_common_info_h "\t{ $flag, \"$flag\" },\n";
	}
	printf $fh_common_info_h "\t{ 0, \"\" }\n};\n\n";
}

sub enum_gen {
	($enum_name) = ($_) =~ /enum (\w+) {/;
	printf $fh_common_info_h "constexpr val_def %s_val_def[] = {\n", $enum_name;
	while (<>) {
		last if $_ =~ /};/;
		($name) = ($_) =~ /\s+(\w+)\s+.*/;
		next if ($name ne uc $name); # skip comments that don't start with *
		next if ($_ =~ /^\s*\/?\s?\*.*/); # skip comments
		next if $name =~ /^\s*$/;  # skip blank lines
		printf $fh_common_info_h "\t{ %s,\t\"%s\" },\n", $name, $name;
	}
	printf $fh_common_info_h "\t{ -1, \"\" }\n};\n\n";
}

sub struct_gen {
	($struct_name) = ($_) =~ /struct (\w+) {/;

	printf $fh_trace_h "void trace_%s_gen(void *ptr, json_object *parent_obj);\n", $struct_name;
	printf $fh_trace_cpp "void trace_%s_gen(void *ptr, json_object *parent_obj)\n{\n", $struct_name;
	printf $fh_trace_cpp "\tjson_object *%s_obj = json_object_new_object();\n", $struct_name;
	printf $fh_trace_cpp "\tstruct %s *p = static_cast<struct %s*>(ptr);\n", $struct_name, $struct_name;

	printf $fh_retrace_h "struct %s *retrace_%s_gen(json_object *ctrl_obj);\n", $struct_name, $struct_name;
	printf $fh_retrace_cpp "struct %s *retrace_%s_gen(json_object *ctrl_obj)\n{\n", $struct_name, $struct_name;
	printf $fh_retrace_cpp "\tstruct %s *p = (struct %s *) calloc(1, sizeof(%s));\n", $struct_name, $struct_name, $struct_name;
	printf $fh_retrace_cpp "\tjson_object *%s_obj;\n", $struct_name;
	# if a key value isn't found, assume it is retracing an element of an array
	printf $fh_retrace_cpp "\tif (!json_object_object_get_ex(ctrl_obj, \"%s\", &%s_obj))\n", $struct_name, $struct_name;
	printf $fh_retrace_cpp "\t\t%s_obj = ctrl_obj;\n", $struct_name;

	while ($line = <> ) {
		chomp($line);
		last if $line =~ /};/;
		next if $line =~ /^\s*$/;  # ignore blank lines
		next if $line =~ /\/\*/; # ignore comment lines
		$line =~ s/^\s+//; # remove leading whitespace
		$line =~ s/;$//; # remove semi-colon at the end
		my @words = split /[\s\[]/, $line; # also split on '[' to avoid arrays
		@words = grep  {/^\D/} @words; # remove values that start with digit from inside []
		@words = grep  {!/\]/} @words; # remove values with brackets e.g. V4L2_H264_REF_LIST_LEN]

		($type) = $words[0];
		$json_type = convert_type_to_json_type($type);

		($member) = $words[scalar @words - 1];
		next if $member =~ /reserved/; # ignore reserved members, they will segfault on retracing

		# generate members that are arrays
		if ($line =~ /.*\[.*/) {
			printf $fh_trace_cpp "\t\/\* %s \*\/\n", $line; #trace comment
			printf $fh_trace_cpp "\tjson_object *%s_obj = json_object_new_array();\n", $member;

			printf $fh_retrace_cpp "\n\t\/\* %s \*\/\n", $line; #retrace comment

			my @dimensions = ($line) =~ /\[(\w+)\]/g;
			$dimensions_count = scalar @dimensions;

			if ($dimensions_count > 1) {
				printf $fh_retrace_cpp "\tint count_%s = 0;\n", $member;
			}
			printf $fh_retrace_cpp "\tjson_object *%s_obj;\n", $member;
			printf $fh_retrace_cpp "\tjson_object_object_get_ex(%s_obj, \"%s\", &%s_obj);\n", $struct_name, $member, $member;

			for (my $idx = 0; $idx < $dimensions_count; $idx = $idx + 1 ) {
				$size = $dimensions[$idx];
				$index_letter = get_index_letter($idx);
				printf $fh_trace_cpp "\t" x ($idx + 1);
				printf $fh_trace_cpp "for (size_t %s = 0; %s < %s\; %s++) {\n", $index_letter, $index_letter, $size, $index_letter;

				printf $fh_retrace_cpp "\t" x ($idx + 1);
				printf $fh_retrace_cpp "for (size_t %s = 0; %s < %s\; %s++) {\n", $index_letter, $index_letter, $size, $index_letter;
			}
			printf $fh_trace_cpp "\t" x ($dimensions_count + 1);
			printf $fh_retrace_cpp "\t" x ($dimensions_count + 1);

			# handle arrays of structs
			if ($type =~ /struct/) {
				my $struct_name_nested = @words[1];
				printf $fh_trace_cpp "json_object *element_obj = json_object_new_object();\n";
				printf $fh_trace_cpp "\t" x ($dimensions_count + 1);
				printf $fh_trace_cpp "trace_%s_gen(&(p->%s", $struct_name_nested, $member;
				for (my $idx = 0; $idx < $dimensions_count; $idx = $idx + 1 ) {
					printf $fh_trace_cpp "[%s]", get_index_letter($idx);
				}
				printf $fh_trace_cpp "), element_obj);\n";
				printf $fh_trace_cpp "\t" x ($dimensions_count + 1);
				printf $fh_trace_cpp "json_object *element_no_key_obj;\n";
				printf $fh_trace_cpp "\t" x ($dimensions_count + 1);
				printf $fh_trace_cpp "json_object_object_get_ex(element_obj, \"%s\", &element_no_key_obj);\n", $struct_name_nested;
				printf $fh_trace_cpp "\t" x ($dimensions_count + 1);
				printf $fh_trace_cpp "json_object_array_add(%s_obj, element_no_key_obj);\n", $member;

				printf $fh_retrace_cpp "p->%s", $member;
				for (my $idx = 0; $idx < $dimensions_count; $idx = $idx + 1) {
					printf $fh_retrace_cpp "[%s]", get_index_letter($idx);
				}
				printf $fh_retrace_cpp " = *retrace_%s_gen(json_object_array_get_idx(%s_obj, ", $struct_name_nested, $member;
				if ($dimensions_count > 1) {
					printf $fh_retrace_cpp "count_%s++", $member;
				} else {
					printf $fh_retrace_cpp "i";
				}
				printf $fh_retrace_cpp "));\n";
			} else {
				printf $fh_trace_cpp "json_object_array_add(%s_obj, json_object_new_%s(p->%s", $member, $json_type, $member;
				for (my $idx = 0; $idx < $dimensions_count; $idx = $idx + 1 ) {
					printf $fh_trace_cpp "[%s]", get_index_letter($idx);
				}
				printf $fh_trace_cpp "));\n";

				printf $fh_retrace_cpp "p->%s", $member;
				for (my $idx = 0; $idx < $dimensions_count; $idx = $idx + 1 ) {
					printf $fh_retrace_cpp "[%s]", get_index_letter($idx);
				}
				printf $fh_retrace_cpp " = ($type) json_object_get_%s(json_object_array_get_idx(%s_obj, ", $json_type, $member;
				if ($dimensions_count > 1) {
					printf $fh_retrace_cpp "count_%s++", $member;
				} else {
					printf $fh_retrace_cpp "i";
				}
				printf $fh_retrace_cpp "));\n";
			}
			#closing brackets for all array types
			for (my $idx = $dimensions_count - 1; $idx >= 0 ; $idx = $idx - 1) {
				printf $fh_trace_cpp "\t" x ($idx + 1);
				printf $fh_trace_cpp "}\n";

				printf $fh_retrace_cpp "\t" x ($idx + 1);
				printf $fh_retrace_cpp "}\n";
			}
			printf $fh_trace_cpp "\tjson_object_object_add(%s_obj, \"%s\", %s_obj);\n\n", $struct_name, $member, $member;
		} elsif ($type =~ /struct/) {
			# member that is a struct but not an array of structs
			my $struct_name_nested = @words[1];
			printf $fh_trace_cpp "\t\/\* %s \*\/\n", $line;
			printf $fh_trace_cpp "\ttrace_%s_gen(&(p->%s), %s_obj);\n", $struct_name_nested, $member, $struct_name;

			printf $fh_retrace_cpp "\t\/\* %s \*\/\n", $line;
			printf $fh_retrace_cpp "\tjson_object *%s_obj;\n", $member;
			printf $fh_retrace_cpp "\tjson_object_object_get_ex(%s_obj, \"%s\", &%s_obj);\n", $struct_name, $struct_name_nested, $member;
			printf $fh_retrace_cpp "\tp->%s = \*retrace_%s_gen(%s_obj);\n\n", $member, $struct_name_nested, $member;
		} else {
			# members that are integers

			if (($struct_name eq "v4l2_ctrl_fwht_params") && ($member =~ /^flags/)) {
				printf $fh_trace_cpp "\tjson_object_object_add(%s_obj, \"flags\", json_object_new_string(fl2s_fwht(p->flags).c_str()));\n", $struct_name;
			} elsif ($member =~ /^flags/) {
				printf $fh_trace_cpp "\tjson_object_object_add(%s_obj, \"flags\", json_object_new_string(fl2s(p->flags, %sflag_def).c_str()));\n", $struct_name, $flag_func_name;
			} else {
				printf $fh_trace_cpp "\tjson_object_object_add(%s_obj, \"%s\", json_object_new_%s(p->%s));\n", $struct_name, $member, $json_type, $member;
			}

			printf $fh_retrace_cpp "\n\tjson_object *%s_obj;\n", $member;
			printf $fh_retrace_cpp "\tjson_object_object_get_ex(%s_obj, \"%s\", &%s_obj);\n", $struct_name, $member, $member;
			if (($struct_name eq "v4l2_ctrl_fwht_params") && ($member =~ /^flags/)) {
				printf $fh_retrace_cpp "\tp->%s = ($type) s2flags_fwht(json_object_get_string(%s_obj));\n", $member, $member, $flag_func_name;
			} elsif ($member =~ /^flags/) {
				printf $fh_retrace_cpp "\tp->%s = ($type) s2flags(json_object_get_string(%s_obj), %sflag_def);\n", $member, $member, $flag_func_name;
			} else {
				printf $fh_retrace_cpp "\tp->%s = ($type) json_object_get_%s(%s_obj);\n", $member, $json_type, $member;
			}
		}
	}
	printf $fh_trace_cpp "\tjson_object_object_add(parent_obj, \"%s\", %s_obj);\n", $struct_name, $struct_name;
	printf $fh_trace_cpp "}\n\n";

	printf $fh_retrace_cpp "\n\treturn p;\n";
	printf $fh_retrace_cpp "}\n\n";
}

open($fh_trace_cpp, '>', 'trace-ctrl-gen.cpp') or die "Could not open trace-ctrl-gen.cpp for writing";
printf $fh_trace_cpp "/* SPDX-License-Identifier: GPL-2.0-only */\n/*\n * Copyright 2022 Collabora Ltd.\n */\n\n";
printf $fh_trace_cpp "#include \"v4l2-tracer-common.h\"\n\n";

open($fh_trace_h, '>', 'trace-ctrl-gen.h') or die "Could not open trace-ctrl-gen.h for writing";
printf $fh_trace_h "/* SPDX-License-Identifier: GPL-2.0-only */\n/*\n * Copyright 2022 Collabora Ltd.\n */\n\n";
printf $fh_trace_h "\#ifndef TRACE_CTRL_GEN_H\n";
printf $fh_trace_h "\#define TRACE_CTRL_GEN_H\n\n";

open($fh_retrace_cpp, '>', 'retrace-ctrls-gen.cpp') or die "Could not open retrace-ctrls-gen.cpp for writing";
printf $fh_retrace_cpp "/* SPDX-License-Identifier: GPL-2.0-only */\n/*\n * Copyright 2022 Collabora Ltd.\n */\n\n";
printf $fh_retrace_cpp "#include \"v4l2-tracer-common.h\"\n#include \"retrace-helper.h\"\n\n";

open($fh_retrace_h, '>', 'retrace-ctrls-gen.h') or die "Could not open retrace-ctrls-gen.h for writing";
printf $fh_retrace_h "/* SPDX-License-Identifier: GPL-2.0-only */\n/*\n * Copyright 2022 Collabora Ltd.\n */\n\n";
printf $fh_retrace_h "\#ifndef RETRACE_CTRLS_GEN_H\n";
printf $fh_retrace_h "\#define RETRACE_CTRLS_GEN_H\n\n";

open($fh_common_info_h, '>', 'v4l2-tracer-info-gen.h') or die "Could not open v4l2-tracer-info-gen.h for writing";
printf $fh_common_info_h "/* SPDX-License-Identifier: GPL-2.0-only */\n/*\n * Copyright 2022 Collabora Ltd.\n */\n\n";
printf $fh_common_info_h "\#ifndef V4L2_TRACER_INFO_GEN_H\n";
printf $fh_common_info_h "\#define V4L2_TRACER_INFO_GEN_H\n\n";
printf $fh_common_info_h "#include \"v4l2-tracer-common.h\"\n\n";

$in_v4l2_controls = true;
@stateless_controls;

while (<>) {
	if (grep {/#define __LINUX_VIDEODEV2_H/} $_) {$in_v4l2_controls = false;}

	if (grep {/^#define.+FWHT_FL_.+/} $_) {
		flag_gen("fwht");
	} elsif (grep {/^#define V4L2_VP8_LF.*/} $_) {
		flag_gen("vp8_loop_filter");
	} elsif (grep {/^#define.+_FL_.+/} $_) {  #use to get media flags
		flag_gen();
	} elsif (grep {/^#define.+_FLAG_.+/} $_) {
		flag_gen();
	}

	# only generate struct functions for v4l2-controls.h
	if ($in_v4l2_controls eq true) {
		if (grep {/^struct/} $_) {
			struct_gen();
		}
	}

	if (grep {/^enum/} $_) {
		enum_gen();
	}

	if (grep {/.*\(V4L2_CID_CODEC_STATELESS_BASE.*/} $_) {
		push (@stateless_controls, $_);
	}

	if (grep {/^\/\* Control classes \*\//} $_) {
		printf $fh_common_info_h "constexpr val_def ctrlclass_val_def[] = {\n";

		while (<>) {
			last if $_ =~ /^\s*$/; #last if blank line
			($ctrl_class) = ($_) =~ /#define\s*(\w+)\s+.*/;
			printf $fh_common_info_h "\t{ %s,\t\"%s\" },\n", $ctrl_class, $ctrl_class;
		}
		printf $fh_common_info_h "\t{ -1, \"\" }\n};\n\n";
	}

	if (grep {/\/\* Values for 'capabilities' field \*\//} $_) {
		printf $fh_common_info_h "constexpr flag_def capabilities_flag_def[] = {\n";
		while (<>) {
			last if $_ =~ /.*V I D E O   I M A G E   F O R M A T.*/;
			next if ($_ =~ /^\/?\s?\*.*/); #skip comments
			next if $_ =~ /^\s*$/; #skip blank lines
			($cap) = ($_) =~ /#define\s+(\w+)\s+.+/;
			printf $fh_common_info_h "\t{ $cap, \"$cap\" },\n"
		}
		printf $fh_common_info_h "\t{ 0, \"\" }\n};\n\n";
	}

	if (grep {/\*      Pixel format         FOURCC                          depth  Description  \*\//} $_) {
		printf $fh_common_info_h "constexpr val_def v4l2_pix_fmt_val_def[] = {\n";
		while (<>) {
			last if $_ =~ /.*SDR formats - used only for Software Defined Radio devices.*/;
			next if ($_ =~ /^\s*\/\*.*/); #skip comments
			next if $_ =~ /^\s*$/; #skip blank lines
			($pixfmt) = ($_) =~ /#define (\w+)\s+.*/;
			printf $fh_common_info_h "\t{ %s,\t\"%s\" },\n", $pixfmt, $pixfmt;
		}
		printf $fh_common_info_h "\t{ -1, \"\" }\n};\n\n";
	}

	if (grep {/.*I O C T L   C O D E S   F O R   V I D E O   D E V I C E S.*/} $_) {
		printf $fh_common_info_h "constexpr val_def ioctl_video_val_def[] = {\n";

		while (<>) {
			next if ($_ =~ /^\/?\s?\*.*/); #skip comments
			next if $_ =~ /^\s*$/; #skip blank lines
			next if $_ =~ /^\s+/; #skip lines that start with a space
			last if $_ =~ /^#define BASE_VIDIOC_PRIVATE/;
			($ioctl_name) = ($_) =~ /^#define\s*(\w+)\s*/;
			printf $fh_common_info_h "\t{ %s,\t\"%s\" },\n", $ioctl_name, $ioctl_name;
		}
		printf $fh_common_info_h "\t{ -1, \"\" }\n};\n\n";
	}

	# from media.h
	if (grep {/\/\* ioctls \*\//} $_) {
		printf $fh_common_info_h "constexpr val_def ioctl_media_val_def[] = {\n";

		while (<>) {
			next if ($_ =~ /^\/?\s?\*.*/); #skip comments
			next if $_ =~ /^\s*$/; #skip blank lines
			next if $_ =~ /^\s+/; #skip lines that start with a space, comments
			last if $_ =~ /^#define MEDIA_ENT_TYPE_SHIFT/;
			($ioctl_name) = ($_) =~ /^#define\s*(\w+)\s*/;
			printf $fh_common_info_h "\t{ %s,\t\"%s\" },\n", $ioctl_name, $ioctl_name;
		}
		printf $fh_common_info_h "\t{ -1, \"\" }\n};\n";
	}
}

printf $fh_common_info_h "constexpr val_def stateless_controls_val_def[] = {\n";
foreach (@stateless_controls) {
	($control) = ($_) =~ /^#define\s*(\w+)\s*/;
	printf $fh_common_info_h "\t{ %s,\t\"%s\" },\n", $control, $control;
}
printf $fh_common_info_h "\t{ -1, \"\" }\n};\n";

printf $fh_trace_h "\n#endif\n";
close $fh_trace_h;
close $fh_trace_cpp;

printf $fh_retrace_h "\n#endif\n";
close $fh_retrace_h;
close $fh_retrace_cpp;

printf $fh_common_info_h "\n#endif\n";
close $fh_common_info_h;
