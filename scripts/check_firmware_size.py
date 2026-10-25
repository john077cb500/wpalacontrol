"""Fail the build if the binary exceeds the ESP8266 board sketch budget."""
from pathlib import Path
Import("env")


def check_size(source, target, env):
    binary = Path(str(target[0]))
    limit = int(env.BoardConfig().get("upload.maximum_size"))
    size = binary.stat().st_size
    report = f"{binary.name}: {size} bytes / {limit} bytes; remaining {limit - size} bytes\n"
    print(report)
    binary.with_suffix(".size.txt").write_text(report)
    if size > limit:
        raise RuntimeError("Experimental firmware exceeds board sketch budget")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", check_size)
