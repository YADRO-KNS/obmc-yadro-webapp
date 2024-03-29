project('obmc-yadro-webapp', 'cpp',
        version : '1.0',
        meson_version: '>=0.53.0',
        default_options: [
            'werror=false',
            'warning_level=3',
            'cpp_std=c++2a',
            'buildtype=debugoptimized',
            'b_ndebug=if-release',
            'b_lto=true',
           ])

# Project related links

project_pretty_name = 'obmc-yadro-webapp'
project_url = 'https://github.com/YADRO-KNS/' + project_pretty_name
project_issues_url = project_url + '/issues/new'
summary('Issues',project_issues_url, section: 'Report Issues')

cmake = import('cmake')
subdir('tools')

# Validate the c++ Standard

if get_option('cpp_std') != 'c++2a'
    error('This project requires c++2a support')
endif

# Get compiler and default build type

cxx = meson.get_compiler('cpp')
build = get_option('buildtype')
optimization = get_option('optimization')
summary('Build Type',build, section : 'Build Info')
summary('Optimization',optimization, section : 'Build Info')

# remove debug information for minsize buildtype
if(get_option('buildtype') == 'minsize')
  add_project_arguments('-DNDEBUG', language : 'cpp')
endif

# Disable lto when compiling with no optimization
if(get_option('optimization') == '0')
  add_project_arguments('-fno-lto', language: 'cpp')
  message('Disabling lto & its supported features as optimization is disabled')
endif

# Include Directories

incdir = include_directories('src')

# Get the options and enable the respective features
## create a MAP of  "options : feature_flag"

if(get_option('tests').enabled())
  summary('unittest','NA', section : 'Enabled Features')
endif

# Add compiler arguments

# -Wpedantic, -Wextra comes by default with warning level
add_project_arguments(
  cxx.get_supported_arguments([
  '-Wold-style-cast',
  '-Wcast-align',
  '-Wunused',
  '-Woverloaded-virtual',
  '-Wconversion',
  '-Wno-attributes',
  ]),
  language: 'cpp'
)

if (cxx.get_id() == 'clang' and cxx.version().version_compare('>9.0'))
add_project_arguments(
  cxx.get_supported_arguments([
    '-Weverything',
    '-Wno-c++98-compat',
    '-Wno-c++98-compat-pedantic',
    '-Wno-global-constructors',
    '-Wno-exit-time-destructors',
    '-Wno-shadow',
    '-Wno-used-but-marked-unused',
    '-Wno-documentation-unknown-command',
    '-Wno-weak-vtables',
    '-Wno-documentation',
    '-Wno-padded',
    '-Wunused-parameter',
    '-Wcovered-switch-default',
    '-Wcomma',
    '-Wextra-semi',
    '-Wzero-as-null-pointer-constant',
    '-Wswitch-enum',
    '-Wnull-dereference',
    '-Wdouble-promotion',
    '-Wformat=2',
  ]),
  language:'cpp')
endif

# if compiler is gnu-gcc , and version is > 8.0 then we add few more
# compiler arguments , we know that will pass

if (cxx.get_id() == 'gcc' and cxx.version().version_compare('>8.0'))

  ## remove once bmcweb/issues/147 is fixed
  add_global_link_arguments('-Wno-stringop-overflow',language:['c','cpp'])
  add_project_arguments('-Wno-stringop-overflow',language:['c','cpp'])

  add_project_arguments(
    cxx.get_supported_arguments([
     '-Wduplicated-cond',
     '-Wduplicated-branches',
     '-Wlogical-op',
     '-Wunused-parameter',
     '-Wnull-dereference',
     '-Wdouble-promotion',
     '-Wformat=2',
     ]),
    language:'cpp')

  if (get_option('buildtype') != 'plain')
    if (get_option('b_lto') == true and get_option('optimization')!='0')
      # Reduce the binary size by removing unnecessary
      # dynamic symbol table entries

      add_project_arguments(
       cxx.get_supported_arguments([
       '-fno-fat-lto-objects',
       '-fvisibility=hidden',
       '-fvisibility-inlines-hidden'
       ]),
       language: 'cpp')

      if cxx.has_link_argument('-Wl,--exclude-libs,ALL')
        add_project_link_arguments('-Wl,--exclude-libs,ALL', language: 'cpp')
      endif
    endif
  endif
endif

# Set Compiler Security flags

security_flags = [
'-fstack-protector-strong',
'-fPIE',
'-fPIC',
'-D_FORTIFY_SOURCE=2',
'-Wformat',
'-Wformat-security'
]

## Add security flags for builds of type 'release','debugoptimized' and 'minsize'

if not (get_option('buildtype') == 'plain' or get_option('buildtype').startswith('debug'))
  add_project_arguments(
   cxx.get_supported_arguments([
    security_flags
  ]),
  language: 'cpp')
endif


# Find the dependency modules, if not found use meson wrap to get them
# automatically during the configure step
obmc_webapp_dependencies = []

atomic =  cxx.find_library('atomic', required: true)
openssl = dependency('openssl', required : true)
threads = dependency('threads')

sdbusplus_dep = dependency('sdbusplus')
systemd = dependency('systemd', required : true)
zlib = dependency('zlib', required : true)
phosphor_logging_dep = dependency('phosphor-logging')
pam = cxx.find_library('pam', required: true)
uuid = dependency('uuid', required: true)

fastcgipp = cxx.find_library('fastcgipp', required : false)
if not fastcgipp.found()
  fastcgipp_proj  = cmake.subproject('fastcgi++', required: true)
  fastcgipp = fastcgipp_proj.dependency('fastcgipp')
endif
obmc_webapp_dependencies += fastcgipp

graphqlparser = cxx.find_library('graphqlparser', required : true)

obmc_webapp_dependencies += [
  atomic,
  openssl,
  systemd,
  zlib,
  threads,
  graphqlparser,
  sdbusplus_dep,
  phosphor_logging_dep,
  pam,
  uuid,
]

if cxx.has_header('nlohmann/json.hpp')
    nlohmann_json = declare_dependency()
else
    subproject('nlohmann', required: false)
    nlohmann_json = declare_dependency(
        include_directories: [
            'subprojects/nlohmann/single_include',
            'subprojects/nlohmann/single_include/nlohmann',
        ]
    )
endif
obmc_webapp_dependencies += nlohmann_json

if get_option('tests').enabled()
  gtest = dependency('gtest', main: true,disabler: true, required : false)
  gmock = dependency('gmock', required : false)
  if not gtest.found() and get_option('tests').enabled()
    gtest_proj = subproject('gtest', required: true)
    gtest = gtest_proj.get_variable('gtest_main_dep')
    gmock = gtest_proj.get_variable('gmock_dep')
  endif
endif

# Source files

srcfiles_obmc_webapp = [
  'src/main.cpp',
  'src/core/application.cpp',
  'src/core/connection.cpp',
  'src/core/response.cpp',
  'src/core/request.cpp',
  'src/core/router.cpp',
  'src/routes.cpp',
  'src/service/session.cpp',
  # protocol handlers
  'src/core/route/handlers/graphql_handler.cpp',
  # entities
  'src/core/entity/entity.cpp',
  'src/core/entity/entity_manager.cpp',
  'src/core/entity/dbus_query.cpp',
  # brokers
  'src/core/connect/dbus_connect.cpp',
  # Redfish
  'src/core/route/redfish/response.cpp',
  'src/core/route/redfish/error_messages.cpp',
]

srcfiles_unittest = [
  'tests/http/headers_utest.cpp'
]

# configure the dbus connection type
dbus_connect_types = {
  'remote': 'BMC_DBUS_CONNECT_REMOTE',
  'system': 'BMC_DBUS_CONNECT_SYSTEM',
}

summary(
  {
    'dbus-connect-type' : get_option('dbus-connect-type')
  },
  section : 'Enabled Features'
)

# Gather the Configuration data
conf_data = configuration_data()
conf_data.set('MESON_INSTALL_PREFIX',get_option('prefix'))
conf_data.set('HTTP_REQ_BODY_LIMIT_MB',get_option('http-body-limit'))
conf_data.set('FASTCGI_SOCKET_PATH', '"' + get_option('fcgi-socket-path') + '"')
conf_data.set('YAWEB_INIT_GUARD_FILE', '"' + get_option('yaweb-init-guard-file') + '"')

if get_option('dbus-connect-type') == 'remote'
  conf_data.set('BMC_DBUS_REMOTE_HOST','"' + get_option('dbus-remote-host') + '"')
  summary(
  {
    'dbus-remote-host' : get_option('dbus-remote-host')
  },
  section : 'Enabled Features'
)
endif

conf_data.set(dbus_connect_types[get_option('dbus-connect-type')], true)

configure_file(output: 'config.h', configuration: conf_data)

# Configure and install systemd unit files

systemd_system_unit_dir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

bindir = get_option('prefix') + '/' +get_option('bindir')

configure_file(input : 'yaweb.service.in',
               output : 'yaweb.service',
               install_dir: systemd_system_unit_dir,
               configuration: conf_data,
               install : true)

summary({
          'prefix' : get_option('prefix'),
          'bindir' : bindir,
          'systemd unit directory' : systemd_system_unit_dir
        }, section : 'Directories')

# Generate the yaweb executable and the test binary
executable('yaweb', srcfiles_obmc_webapp,
            include_directories : incdir,
            dependencies: obmc_webapp_dependencies,
            install: true,
            install_dir:bindir)

if(get_option('tests').enabled())
  foreach src_test : srcfiles_unittest
    testname = src_test.split('/')[-1].split('.')[0]
    test(testname,executable(testname,src_test,
                include_directories : incdir,
                install_dir: bindir,
                dependencies: [ gtest,openssl,gmock,nlohmann_json,fastcgipp]))
  endforeach
endif
