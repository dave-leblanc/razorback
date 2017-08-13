# Filters added to this controller apply to all controllers in the application.
# Likewise, all the methods added will be available for all controllers.
require 'sys/proctable'
include Sys

class ApplicationController < ActionController::Base
  protect_from_forgery # See ActionController::RequestForgeryProtection for details
  before_filter :check_database

  # Scrub sensitive parameters from your log
  # filter_parameter_logging :password

  protected
  def session_cleanup
		backup = session.data.clone
		reset_session
		backup.to_hash.each { |k, v|
			session[k] = v unless k.is_a?(String) && k =~ /^as:/
		}
  end

	def check_database
		begin
			session[:database_state] = ConfigOption.RAZORBACK_PATH.name if session[:database_state].nil?
		rescue Exception => e
			render :text => 'Razorback configuration options do not exist.  Did you run rake db:migrate?', :layout => false
		end
	end

   def restart_masterNugget
      begin
         ProcTable.ps do |p|
            Process.kill("HUP", p.pid) if p.name =~ /masterNugget/i
         end

         flash[:info] = 'Your masterNugget has been restarted'

      rescue Errno::EPERM
         flash[:error] = 'Your apache user does not have the correct permissions to restart your masterNugget process'
      rescue Exception => e
         flash[:error] = "An unknown exception has occurred: #{e.to_s}"
      end
   end

end
