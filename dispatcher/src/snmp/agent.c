#include "config.h"
#include "snmp/snmp-config.h"
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "dataTypesTable.h"
#include <razorback/log.h>
#include <razorback/thread.h>

void 
SNMP_Agent_Thread(struct Thread *thread)
{
    rzb_log(LOG_INFO, "SNMP Agent Started");
    snmp_enable_stderrlog();
    netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);
    SOCK_STARTUP;
    init_agent("razorback");

    init_dataTypesRoutingTable();
      init_snmp("razorback");

    while (!Thread_IsStopped(thread))
    {
        agent_check_and_process(1);
    }
    snmp_shutdown("razorback");
    SOCK_CLEANUP;
    rzb_log(LOG_INFO, "SNMP Agent Exited");
}
