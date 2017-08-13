#!/usr/bin/ruby -w

###########################################
#
#   Ruby Library for Razorback
#   7/14/2011
#
###########################################

module Razorback
    def Razorback.register(id, type, *types)
        puts "<?xml version=\"1.0\"?>"
        puts "<razorback>\n\t<registration>"
        puts "\t\t<nugget_id>#{id}</nugget_id>"
        puts "\t\t<application_type>#{type}</application_type>"
        puts "\t\t<data_types>"
        types.each{|i| puts "\t\t\t<data_type>#{i}</data_type>"}
        puts "\t\t</data_types>"
        puts "\t</registration>\n</razorback>"
    end

    class Response
        def initialize()
            @preamble = 0
            @alert_preamble = 0
        end
        def start()
            puts "<?xml version=\"1.0\"?>"
            puts "<razorback>"
            puts "\t<response>"
            @preamble = 1
        end
        def log(severity, message)
            if @preamble == 0
                puts "FATAL: Failed to start response"
                exit
            end
            puts "\t\t<log level=\"#{severity}\">"
            puts "\t\t\t<message>#{message}</message>"
            puts "\t\t</log>"
        end
        def alertStart(severity, gid, sid, message, sf_flags, sf_unset, ent_flags, ent_unset)
            if @preamble == 0
                puts "FATAL: Failed to start response XML"
                exit
            end
            @alert_preamble = 1
            puts "\t\t<verdict priority=\"#{severity}\" gid=\"#{gid}\" sid=\"#{sid}\">"
            puts "\t\t\t<flags>"
            puts "\t\t\t\t<sourcefire>"
            puts "\t\t\t\t\t<set>#{sf_flags}</set>"
            puts "\t\t\t\t\t<unset>#{sf_unset}</unset>"
            puts "\t\t\t\t</sourcefire>"
            puts "\t\t\t\t<enterprise>"
            puts "\t\t\t\t\t<set>#{ent_flags}</set>"
            puts "\t\t\t\t\t<unset>#{ent_unset}</unset>"
            puts "\t\t\t\t</enterprise>"
            puts "\t\t\t</flags>"
            puts "\t\t\t<message>#{message}</message>"
            puts "\t\t\t<metadata>"
        end
        def alertData(type,data)
            if @alert_preamble == 0
                puts "FATAL: Failed to start alert XML"
                exit
            end
            puts "\t\t\t\t<entry>"
            puts "\t\t\t\t\t<type>#{type}</type>"
            puts "\t\t\t\t\t<data>#{data}</data>"
            puts "\t\t\t\t</entry>"
        end
        def alertEnd()
            puts "\t\t\t</metadata>"
            puts "\t\t</verdict>"
            @alert_preamble = 0
        end
        def end()
            puts "\t</response>"
            puts "</razorback>"
            @preamble = 0
        end
    end

### VERDICT VALUES
NO_FLAGS             = 0x00000000
FLAG_GOOD            = 0x00000001
FLAG_BAD             = 0x00000002

### ALERT VALUES
HIGH                 = 0x00000001
MEDIUM               = 0x00000002
LOW                  = 0x00000004
### 
end
