/*
 * Note: this file originally auto-generated by mib2c using
 *  : mib2c.iterate.conf 17821 2009-11-11 09:00:00Z dts12 $
 */
#include "config.h"
#include "snmp/snmp-config.h"
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "dataTypesTable.h"
#include "statistics.h"
#include <razorback/log.h>


/** Initializes the dataTypesTable module */
void
init_dataTypesRoutingTable(void)
{
  /* here we initialize all the tables we're planning on supporting */
    initialize_table_dataTypesRoutingTable();
}


/** Initialize the dataTypesRoutingTable table by defining its contents and how it's structured */
void
initialize_table_dataTypesRoutingTable(void)
{
    static oid dataTypesRoutingTable_oid[] = {1,3,6,1,4,1,14223,3,1,2,1};
    size_t dataTypesRoutingTable_oid_len   = OID_LENGTH(dataTypesRoutingTable_oid);
    netsnmp_handler_registration    *reg;
    netsnmp_iterator_info           *iinfo;
    netsnmp_table_registration_info *table_info;

    reg = netsnmp_create_handler_registration(
              "dataTypesRoutingTable",     dataTypesRoutingTable_handler,
              dataTypesRoutingTable_oid, dataTypesRoutingTable_oid_len,
              HANDLER_CAN_RONLY
              );

    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_OCTET_STR,  /* index: dataTypeName */
                           0);
    table_info->min_column = COLUMN_DATATYPENAME;
    table_info->max_column = COLUMN_DATATYPEERROR;
    
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->get_first_data_point = dataTypesRoutingTable_get_first_data_point;
    iinfo->get_next_data_point  = dataTypesRoutingTable_get_next_data_point;
    iinfo->table_reginfo        = table_info;
    
    netsnmp_register_table_iterator( reg, iinfo );
    if (!reg || !table_info || !iinfo) {
        rzb_log(LOG_ERR,
                 "malloc failed in initialize_table_managementTable");
        return; /** Serious error. */
    }

    /* Initialise the contents of the table here */
}



/* Example iterator hook routines - using 'get_next' to do most of the work */
netsnmp_variable_list *
dataTypesRoutingTable_get_first_data_point(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    Statistics_Lock_DataTypeList();
    *my_loop_context = g_stats_dataTypeHead;
    Statistics_Unlock_DataTypeList();
    return dataTypesRoutingTable_get_next_data_point(my_loop_context, my_data_context,
                                    put_index_data,  mydata );
}

netsnmp_variable_list *
dataTypesRoutingTable_get_next_data_point(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    struct DataTypeStats *entry = (struct DataTypeStats *)*my_loop_context;
    netsnmp_variable_list *idx = put_index_data;

    if ( entry ) {
        snmp_set_var_value( idx, (u_char *)entry->name, sizeof(entry->name) );
        idx = idx->next_variable;
        *my_data_context = (void *)entry;
        *my_loop_context = (void *)entry->next;
        return put_index_data;
    } else {
        return NULL;
    }
}


/** handles requests for the dataTypesRoutingTable table */
int
dataTypesRoutingTable_handler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests) {

    netsnmp_request_info       *request;
    netsnmp_table_request_info *table_info;
    netsnmp_variable_list *requestvb;
    struct DataTypeStats          *table_entry;

    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request=requests; request; request=request->next) {
            table_entry = (struct DataTypeStats *)
                              netsnmp_extract_iterator_context(request);
            table_info  =     netsnmp_extract_table_info(      request);
            requestvb = request->requestvb; 
            if (table_entry == NULL) {
                continue;
            }
            switch (table_info->colnum) {
            case COLUMN_DATATYPENAME:
                snmp_set_var_typed_value(requestvb, ASN_OCTET_STR, (u_char *)table_entry->name, strlen(table_entry->name) );
                break;
            case COLUMN_DATATYPEREQUESTS:
                snmp_set_var_typed_value(requestvb, ASN_COUNTER, (u_char *)&table_entry->routingRequestCount, sizeof(table_entry->routingRequestCount) );
                break;
            case COLUMN_DATATYPEERROR:
                snmp_set_var_typed_value(requestvb, ASN_COUNTER, (u_char *)&table_entry->routingSuccessCount, sizeof(table_entry->routingRequestCount) );
                break;
            case COLUMN_DATATYPESUCCESS:
                snmp_set_var_typed_value(requestvb, ASN_COUNTER, (u_char *)&table_entry->routingSuccessCount, sizeof(table_entry->routingRequestCount) );
                break;
            default:
                netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHOBJECT);
                break;
            }
        }
        break;

    }
    return SNMP_ERR_NOERROR;
}