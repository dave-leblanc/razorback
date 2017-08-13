class StatusController < ApplicationController
	before_filter :session_cleanup, :only => :index

	def index
	end

	def restart
		restart_masterNugget

		redirect_to :back
	end
end
