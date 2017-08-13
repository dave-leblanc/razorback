class CreateRoutingCounts < ActiveRecord::Migration
  def self.up
    create_table :routing_counts do |t|
      t.column :id, :integer
      t.column :Data_Type_UUID, :string
      t.column :count, :integer
      t.timestamps
    end
  end

  def self.down
    drop_table :routing_counts
  end
end
