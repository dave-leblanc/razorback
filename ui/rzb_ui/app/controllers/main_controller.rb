class MainController < ApplicationController
	def index
		@data_types = [DataType.new(:Name => "USE_MAGIC", :Description => "Use File Magic")]
		@data_types += DataType.all
		@data_types.delete(DataType.find_by_Name("ANY_DATA"))
	end
end
