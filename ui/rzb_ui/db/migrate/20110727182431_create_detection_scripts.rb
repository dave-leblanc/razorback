class CreateDetectionScripts < ActiveRecord::Migration
  def self.up
    create_table :detection_scripts do |t|
		t.column :name, :string
		t.column :upload_path, :string
		t.column :post_command, :string
    end

	DetectionScript.create(:name => 'Yara', :upload_path => '/tmp/yara')
	DetectionScript.create(:name => 'ClamAV', :upload_path => '/tmp/clamav')
  end

  def self.down
    drop_table :detection_scripts
  end
end
