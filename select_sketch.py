# PlatformIO pre-script: build the sketch folder given by `custom_sketch_dir`.
#
# PlatformIO only converts .ino files at the top level of src_dir, so the
# per-board Arduino IDE sketch folders under firmware/ cannot be selected with
# build_src_filter. Instead, point the source directory of each environment at
# its own sketch folder.
Import("env")

sketch_dir = env.GetProjectOption("custom_sketch_dir")
env.Replace(PROJECT_SRC_DIR=env.subst("$PROJECT_DIR/" + sketch_dir))
