class CreateConfigOptions < ActiveRecord::Migration

  def self.up
    create_table :config_options do |t|
		t.column :name, :string
		t.column :value, :string
		t.column :required, :boolean, :default => false
    end

	ConfigOption.create(:name => 'RAZORBACK_PATH', :value => '/opt/razorback', :required => true)
	ConfigOption.create(:name => 'RAZORBACK_CONSOLE_USERNAME', :value => 'razorback', :required => true)
	ConfigOption.create(:name => 'RAZORBACK_CONSOLE_PASSWORD', :value => 'razorback', :required => true)
	ConfigOption.create(:name => 'RAZORBACK_CONSOLE_HOSTNAME', :value => '127.0.0.1', :required => true)
	ConfigOption.create(:name => 'RAZORBACK_CONSOLE_PORT', :value => '10001', :required => true)
	ConfigOption.create(:name => 'RAZORBACK_CONSOLE_STATUS_COMMAND', :value => 'show nugget status', :required => true)
	ConfigOption.create(:name => 'RAZORBACK_CONSOLE_STATUS_UPDATE_INTERVAL', :value => '5', :required => true)
	ConfigOption.create(:name => 'RAZORBACK_ROUTING_STATS_PATH', :value => '/tmp/rzb.stats', :required => true)
	ConfigOption.create(:name => 'RAZORBACK_ROUTING_ROUTING_TABLE_COMMAND', :value => 'show routing summary', :required => true)

  end

  def self.down
    drop_table :config_options
  end
end
