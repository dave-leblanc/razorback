class CreateNuggetStatuses < ActiveRecord::Migration
  def self.up
    create_table :nugget_statuses do |t|
	  t.column :id, :integer
	  t.column :Nugget_UUID, :string
	  t.column :status, :string
	  t.column :deadtime, :integer
      t.timestamps
    end
  end

  def self.down
    drop_table :nugget_statuses
  end
end
