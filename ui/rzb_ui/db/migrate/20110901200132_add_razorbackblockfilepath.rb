class AddRazorbackblockfilepath < ActiveRecord::Migration
  def self.up
  		ConfigOption.create(:name => 'RAZORBACK_BLOCK_FILE_PATH', :value => '/tmp', :required => true)
  end

  def self.down
  end
end
