# This file is auto-generated from the current state of the database. Instead of editing this file, 
# please use the migrations feature of Active Record to incrementally modify your database, and
# then regenerate this schema definition.
#
# Note that this schema.rb definition is the authoritative source for your database schema. If you need
# to create the application database on another system, you should be using db:schema:load, not running
# all the migrations from scratch. The latter is a flawed and unsustainable approach (the more migrations
# you'll amass, the slower it'll run and the greater likelihood for issues).
#
# It's strongly recommended to check this file into your version control system.

ActiveRecord::Schema.define(:version => 20110901200132) do

  create_table "Alert", :primary_key => "Alert_ID", :force => true do |t|
    t.string  "Nugget_UUID",   :limit => 36,                :null => false
    t.integer "Event_ID",      :limit => 8
    t.integer "Block_ID",      :limit => 8
    t.integer "Metabook_ID",   :limit => 8
    t.integer "Time_Secs",     :limit => 8,  :default => 0, :null => false
    t.integer "Time_NanoSecs", :limit => 8,  :default => 0, :null => false
    t.integer "GID"
    t.integer "SID"
    t.integer "Priority",      :limit => 1,  :default => 0
    t.text    "Message",                                    :null => false
  end

  add_index "Alert", ["Block_ID"], :name => "Alert_FKIndex1"
  add_index "Alert", ["Event_ID"], :name => "Alert_FKIndex2"
  add_index "Alert", ["Metabook_ID"], :name => "Alert_FKIndex4"
  add_index "Alert", ["Nugget_UUID"], :name => "Alert_FKIndex3"

  create_table "App_Type", :primary_key => "UUID", :force => true do |t|
    t.string "Nugget_Type_UUID", :limit => 36,  :null => false
    t.string "Name",             :limit => 100
    t.text   "Description"
  end

  add_index "App_Type", ["Nugget_Type_UUID"], :name => "App_Type_FKIndex1"

  create_table "Block", :primary_key => "Block_ID", :force => true do |t|
    t.string    "Data_Type_UUID", :limit => 36,                :null => false
    t.binary    "Hash",                                        :null => false
    t.integer   "Size",                                        :null => false
    t.integer   "Metabook_ID",    :limit => 8
    t.integer   "SF_Flags",                     :default => 0, :null => false
    t.integer   "ENT_Flags",                    :default => 0, :null => false
    t.timestamp "Timestamp",                                   :null => false
  end

  add_index "Block", ["Data_Type_UUID"], :name => "Block_FKIndex2"
  add_index "Block", ["Hash", "Size"], :name => "Block_Size_Hash_Key", :unique => true, :length => {"Size"=>nil, "Hash"=>"512"}
  add_index "Block", ["Hash"], :name => "Block_Hash_Index", :length => {"Hash"=>"512"}
  add_index "Block", ["Metabook_ID"], :name => "Block_FKIndex1"
  add_index "Block", ["Size"], :name => "Block_Size_Index"

  create_table "Block_Inspection", :id => false, :force => true do |t|
    t.integer "Block_ID",            :limit => 8,                 :null => false
    t.string  "Inspector_Type_UUID", :limit => 36,                :null => false
    t.string  "Inspector_UUID",      :limit => 36
    t.integer "Status",              :limit => 1,  :default => 0, :null => false
  end

  add_index "Block_Inspection", ["Block_ID", "Inspector_UUID", "Status"], :name => "Block_Inspection_ID_Insp_Status"
  add_index "Block_Inspection", ["Block_ID"], :name => "Block_Inspection_ibfk_1"
  add_index "Block_Inspection", ["Inspector_Type_UUID"], :name => "Block_Inspection_ibfk_2"
  add_index "Block_Inspection", ["Inspector_UUID"], :name => "Block_Inspection_ibfk_3"

  create_table "Connection_Info", :primary_key => "Metabook_ID", :force => true do |t|
    t.integer "Source_Address_ID",      :limit => 8
    t.integer "Destination_Address_ID", :limit => 8
    t.integer "Source_Port"
    t.integer "Destination_Port"
    t.integer "Protocol"
  end

  add_index "Connection_Info", ["Destination_Address_ID"], :name => "Address_Info_FKIndex2"
  add_index "Connection_Info", ["Metabook_ID"], :name => "Address_Info_FKIndex3"
  add_index "Connection_Info", ["Source_Address_ID"], :name => "Address_Info_FKIndex1"

  create_table "Event", :primary_key => "Event_ID", :force => true do |t|
    t.string  "Nugget_UUID",   :limit => 36,                :null => false
    t.integer "Block_ID",      :limit => 8
    t.integer "Metabook_ID",   :limit => 8
    t.integer "Time_Secs",     :limit => 8,  :default => 0, :null => false
    t.integer "Time_NanoSecs", :limit => 8,  :default => 0, :null => false
  end

  add_index "Event", ["Block_ID"], :name => "Event_FKIndex1"
  add_index "Event", ["Metabook_ID"], :name => "Event_FKIndex3"
  add_index "Event", ["Nugget_UUID"], :name => "Event_FKIndex2"

  create_table "Host", :primary_key => "Host_ID", :force => true do |t|
    t.string    "Name"
    t.integer   "SF_Flags",                 :default => 0, :null => false
    t.integer   "ENT_Flags",                :default => 0, :null => false
    t.integer   "Metabook_ID", :limit => 8
    t.timestamp "Timestamp",                               :null => false
  end

  add_index "Host", ["Metabook_ID"], :name => "Host_FKIndex1"
  add_index "Host", ["Name"], :name => "Host_Name", :unique => true

  create_table "IP_Address", :primary_key => "IP_Address_ID", :force => true do |t|
    t.string    "Address_Type", :limit => 0,                  :null => false
    t.binary    "Address",      :limit => 255,                :null => false
    t.integer   "SF_Flags",                    :default => 0, :null => false
    t.integer   "ENT_Flags",                   :default => 0, :null => false
    t.integer   "Metabook_ID",  :limit => 8
    t.timestamp "Timestamp",                                  :null => false
  end

  add_index "IP_Address", ["Address"], :name => "IP_Address_Address", :length => {"Address"=>"16"}
  add_index "IP_Address", ["Address_Type", "Address"], :name => "IP_Address_Address_Type", :unique => true, :length => {"Address"=>"16", "Address_Type"=>nil}
  add_index "IP_Address", ["Address_Type"], :name => "IP_Address_Type"
  add_index "IP_Address", ["Metabook_ID"], :name => "IP_Address_FKIndex1"

  create_table "LK_Block_Block", :id => false, :force => true do |t|
    t.integer   "Metabook_ID", :limit => 8
    t.integer   "Parent_ID",   :limit => 8,                :null => false
    t.integer   "Child_ID",    :limit => 8,                :null => false
    t.integer   "SF_Flags",                 :default => 0, :null => false
    t.integer   "ENT_Flags",                :default => 0, :null => false
    t.timestamp "Timestamp",                               :null => false
  end

  add_index "LK_Block_Block", ["Child_ID"], :name => "LK_Block_Block_FKIndex2"
  add_index "LK_Block_Block", ["Metabook_ID"], :name => "LK_Block_Block_FKIndex3"
  add_index "LK_Block_Block", ["Parent_ID", "Child_ID"], :name => "LK_Block_Block_Primary"
  add_index "LK_Block_Block", ["Parent_ID"], :name => "LK_Block_Block_FKIndex1"

  create_table "LK_Event_Event", :id => false, :force => true do |t|
    t.integer "Metabook_ID", :limit => 8
    t.integer "Parent_ID",   :limit => 8, :null => false
    t.integer "Child_ID",    :limit => 8, :null => false
  end

  add_index "LK_Event_Event", ["Child_ID"], :name => "LK_Event_Event_FKIndex2"
  add_index "LK_Event_Event", ["Metabook_ID"], :name => "LK_Event_Event_FKIndex3"
  add_index "LK_Event_Event", ["Parent_ID", "Child_ID"], :name => "LK_Event_Event_Primary"
  add_index "LK_Event_Event", ["Parent_ID"], :name => "LK_Event_Event_FKIndex1"

  create_table "LK_Event_Host", :primary_key => "Metabook_ID", :force => true do |t|
    t.integer "Host_ID", :limit => 8, :null => false
  end

  add_index "LK_Event_Host", ["Host_ID"], :name => "LK_Metabook_Host_FKIndex2"
  add_index "LK_Event_Host", ["Host_ID"], :name => "LK_Metabook_Host_ID", :unique => true
  add_index "LK_Event_Host", ["Metabook_ID"], :name => "LK_Metabook_Host_FKIndex1"

  create_table "LK_Metabook_URI", :primary_key => "Metabook_ID", :force => true do |t|
    t.integer "URI_ID", :limit => 8, :null => false
  end

  add_index "LK_Metabook_URI", ["Metabook_ID"], :name => "LK_Metabook_URI_FKIndex1"
  add_index "LK_Metabook_URI", ["URI_ID"], :name => "LK_Metabook_URI_FKIndex2"

  create_table "Log_Message", :primary_key => "Message_ID", :force => true do |t|
    t.string    "Nugget_UUID", :limit => 36,                :null => false
    t.integer   "Event_ID",    :limit => 8
    t.timestamp "Timestamp",                                :null => false
    t.text      "Message",                                  :null => false
    t.integer   "Priority",    :limit => 1,  :default => 0, :null => false
  end

  add_index "Log_Message", ["Event_ID"], :name => "Alert_FKIndex1"
  add_index "Log_Message", ["Nugget_UUID"], :name => "Alert_FKIndex2"

  create_table "Metabook", :primary_key => "Metabook_ID", :force => true do |t|
  end

  create_table "Metadata", :primary_key => "Metadata_ID", :force => true do |t|
    t.string  "Metadata_Type_UUID", :limit => 36, :null => false
    t.string  "Metadata_Name_UUID", :limit => 36, :null => false
    t.integer "Metabook_ID",        :limit => 8,  :null => false
    t.binary  "Metadata"
  end

  add_index "Metadata", ["Metabook_ID"], :name => "Metadata_Metabook"
  add_index "Metadata", ["Metadata_Name_UUID"], :name => "Metadata_FKIndex2"
  add_index "Metadata", ["Metadata_Type_UUID"], :name => "Metadata_FKIndex3"

  create_table "Nugget", :primary_key => "UUID", :force => true do |t|
    t.string  "App_Type_UUID",    :limit => 36
    t.string  "Nugget_Type_UUID", :limit => 36,                 :null => false
    t.string  "Name",             :limit => 100
    t.text    "Location"
    t.text    "Contact"
    t.text    "Notes"
    t.integer "State",                           :default => 0, :null => false
  end

  add_index "Nugget", ["App_Type_UUID"], :name => "Nugget_FKIndex1"
  add_index "Nugget", ["Nugget_Type_UUID"], :name => "Nugget_FKIndex2"

  create_table "Nugget_Type", :primary_key => "UUID", :force => true do |t|
    t.string "Name",        :limit => 100
    t.text   "Description"
  end

  create_table "Protocol", :primary_key => "Protocol_ID", :force => true do |t|
    t.string "Name", :limit => 100, :null => false
  end

  add_index "Protocol", ["Name"], :name => "Protocol_Name", :unique => true

  create_table "URI", :primary_key => "URI_ID", :force => true do |t|
    t.text      "Path"
    t.text      "File_Name"
    t.integer   "Protocol_ID",                             :null => false
    t.integer   "SF_Flags",                 :default => 0, :null => false
    t.integer   "ENT_Flags",                :default => 0, :null => false
    t.integer   "Metabook_ID", :limit => 8
    t.timestamp "Timestamp",                               :null => false
  end

  add_index "URI", ["File_Name"], :name => "URI_File", :length => {"File_Name"=>"200"}
  add_index "URI", ["Metabook_ID"], :name => "URI_FKIndex2"
  add_index "URI", ["Path", "File_Name", "Protocol_ID"], :name => "URI_Path_Name_Proto", :unique => true, :length => {"Protocol_ID"=>nil, "File_Name"=>"200", "Path"=>"200"}
  add_index "URI", ["Path"], :name => "URI_Path", :length => {"Path"=>"200"}
  add_index "URI", ["Protocol_ID"], :name => "URI_FKIndex1"
  add_index "URI", ["Protocol_ID"], :name => "URI_Proto"

  create_table "UUID_Data_Type", :primary_key => "UUID", :force => true do |t|
    t.string "Name",        :limit => 32
    t.text   "Description"
  end

  add_index "UUID_Data_Type", ["Name"], :name => "UUID_Data_Type_Name", :unique => true

  create_table "UUID_Metadata_Name", :primary_key => "UUID", :force => true do |t|
    t.string "Name",        :limit => 32, :null => false
    t.text   "Description"
  end

  add_index "UUID_Metadata_Name", ["Name"], :name => "UUID_MetadataName_Name", :unique => true

  create_table "UUID_Metadata_Type", :primary_key => "UUID", :force => true do |t|
    t.string  "Name",        :limit => 32
    t.text    "Description"
    t.integer "Data_Size"
  end

  add_index "UUID_Metadata_Type", ["Name"], :name => "UUIDs_Name"

  create_table "config_options", :force => true do |t|
    t.string  "name"
    t.string  "value"
    t.boolean "required", :default => false
  end

  create_table "detection_scripts", :force => true do |t|
    t.string "name"
    t.string "upload_path"
    t.string "post_command"
  end

  create_table "nugget_statuses", :force => true do |t|
    t.string   "Nugget_UUID"
    t.string   "status"
    t.integer  "deadtime"
    t.datetime "created_at"
    t.datetime "updated_at"
  end

  create_table "routing_counts", :force => true do |t|
    t.string   "Data_Type_UUID"
    t.integer  "count"
    t.datetime "created_at"
    t.datetime "updated_at"
  end

  create_table "routing_instances", :force => true do |t|
    t.string   "Data_Type_UUID"
    t.string   "App_Type_UUID"
    t.datetime "created_at"
    t.datetime "updated_at"
  end

  create_table "system_statuses", :force => true do |t|
    t.string   "name"
    t.string   "status"
    t.string   "command"
    t.string   "confirmation"
    t.datetime "created_at"
    t.datetime "updated_at"
  end

end
