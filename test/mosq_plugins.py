import mosq_test
import platform
from pathlib import Path

def gen_test_plugin_path(name):
    if platform.system() == "Windows":
        libending = "dll"
    else:
        libending = "so"

    return Path(
        mosq_test.get_build_root(),
        "test",
        "broker",
        "c",
        mosq_test.get_build_type(),
        f"{name}.{libending}",
)

def gen_plugin_path(folder, name):
    if platform.system() == "Windows":
        libending = "dll"
    else:
        libending = "so"

    return Path(
        mosq_test.get_build_root(),
        "plugins",
        folder,
        mosq_test.get_build_type(),
        f"{name}.{libending}",
)

ACL_FILE_PLUGIN_PATH = gen_plugin_path("acl-file", "mosquitto_acl_file")
DYNSEC_PLUGIN_PATH = gen_plugin_path("dynamic-security", "mosquitto_dynamic_security")
PASSWORD_FILE_PLUGIN_PATH = gen_plugin_path("password-file", "mosquitto_password_file")
PERSIST_SQLITE_PLUGIN_PATH = gen_plugin_path("persist-sqlite", "mosquitto_persist_sqlite")
SPARKPLUG_AWARE_PLUGIN_PATH = gen_plugin_path("sparkplug-aware", "mosquitto_sparkplug_aware")