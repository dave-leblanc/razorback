class CreateSystemStatuses < ActiveRecord::Migration
  def self.up
    create_table :system_statuses do |t|
		t.column :id, :integer
		t.column :name, :string
		t.column :status, :string
		t.column :command, :string
		t.column :confirmation, :string
		t.timestamps
    end
	
	SystemStatus.create(:name => 'MySQL', :command => '/usr/sbin/service mysql status', :confirmation => 'running')
	SystemStatus.create(:name => 'memcached', :command => '/usr/sbin/service memcached status', :confirmation => 'running')
	SystemStatus.create(:name => 'ActiveMQ', :command => 'ps aux | grep activemq | grep -v grep', :confirmation => 'activemq' )
	SystemStatus.create(:name => 'Razorback Dispatcher', :command => 'ps aux | grep dispatcher | grep -v grep', :confirmation => 'dispatcher' )
	SystemStatus.create(:name => 'Razorback masterNugget', :command => 'ps aux | grep masterNugget | grep -v grep', :confirmation => 'masterNugget' )
  end

  def self.down
    drop_table :system_statuses
  end
end
