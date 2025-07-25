build_lib(
  LIBNAME applications
  SOURCE_FILES
    helper/bulk-send-helper.cc
    helper/on-off-helper.cc
    helper/packet-sink-helper.cc
    helper/three-gpp-http-helper.cc
    helper/udp-client-server-helper.cc
    helper/udp-echo-helper.cc
    helper/node-app-helper.cc
    helper/checkpoint-helper.cc
    helper/udp-helper.cc
    helper/config-helper.cc
    model/application-packet-probe.cc
    model/bulk-send-application.cc
    model/onoff-application.cc
    model/packet-loss-counter.cc
    model/packet-sink.cc
    model/seq-ts-echo-header.cc
    model/seq-ts-header.cc
    model/seq-ts-size-header.cc
    model/three-gpp-http-client.cc
    model/three-gpp-http-header.cc
    model/three-gpp-http-server.cc
    model/three-gpp-http-variables.cc
    model/udp-client.cc
    model/udp-echo-client.cc
    model/udp-echo-server.cc
    model/udp-server.cc
    model/udp-trace-client.cc
    model/apps/server-node-app.cc
    model/apps/client-node-app.cc
    model/energy/battery.cc
    model/energy/energy-generator.cc
    model/energy/fixed-energy-generator.cc
    model/energy/circular-energy-generator.cc
    model/protocols/checkpoint-strategy.cc
    model/protocols/decentralized-recovery-protocol.cc
    model/protocols/global-sync-clocks-strategy.cc
    model/protocols/efficient-assync-recovery-protocol.cc
    model/protocols/earp-without-rollback.cc
    model/protocols/earp-without-rollback-v2.cc
    model/apps/checkpoint-app.cc
    model/data/message-data.cc
    exception/node-asleep-exception.cc
    exception/node-depleted-exception.cc
    util/json-utils.cc
    util/log-utils.cc
    util/utils.cc
  HEADER_FILES
    helper/bulk-send-helper.h
    helper/on-off-helper.h
    helper/packet-sink-helper.h
    helper/three-gpp-http-helper.h
    helper/udp-client-server-helper.h
    helper/udp-echo-helper.h
    helper/node-app-helper.h
    helper/checkpoint-helper.h
    helper/udp-helper.h
    helper/config-helper.h
    model/application-packet-probe.h
    model/bulk-send-application.h
    model/onoff-application.h
    model/packet-loss-counter.h
    model/packet-sink.h
    model/seq-ts-echo-header.h
    model/seq-ts-header.h
    model/seq-ts-size-header.h
    model/three-gpp-http-client.h
    model/three-gpp-http-header.h
    model/three-gpp-http-server.h
    model/three-gpp-http-variables.h
    model/udp-client.h
    model/udp-echo-client.h
    model/udp-echo-server.h
    model/udp-server.h
    model/udp-trace-client.h
    model/data/message-data.h
    model/apps/server-node-app.h
    model/apps/client-node-app.h
    model/apps/checkpoint-app.h
    model/energy/battery.h
    model/energy/energy-generator.h
    model/energy/fixed-energy-generator.h
    model/energy/circular-energy-generator.h
    model/protocols/checkpoint-strategy.h
    model/protocols/global-sync-clocks-strategy.h
    model/protocols/decentralized-recovery-protocol.h
    model/protocols/efficient-assync-recovery-protocol.h
    model/protocols/earp-without-rollback.h
    model/protocols/earp-without-rollback-v2.h
    model/application-type.h
    exception/node-asleep-exception.h
    exception/node-depleted-exception.h
    util/json-utils.h
    util/log-utils.h
    util/utils.h
  LIBRARIES_TO_LINK ${libinternet}
  TEST_SOURCES
    test/three-gpp-http-client-server-test.cc
    test/bulk-send-application-test-suite.cc
    test/udp-client-server-test.cc
)
