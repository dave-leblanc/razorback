class CreateRoutingInstances < ActiveRecord::Migration
  def self.up
    create_table :routing_instances do |t|
		t.column :id, :integer
		t.column :Data_Type_UUID, :string
		t.column :App_Type_UUID, :string
      t.timestamps
    end
  end

  def self.down
    drop_table :routing_instances
  end
end
