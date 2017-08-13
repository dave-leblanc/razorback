class UploadsController < ApplicationController
	before_filter :session_cleanup

	def upload_block
        begin
			fileInject_path = "#{ConfigOption.RAZORBACK_PATH.value}/bin/fileInject"
			tmp_file = "#{File.dirname(params[:block]['upload'].path)}/#{params[:block]['upload'].original_filename}"
			FileUtils.mv(params[:block]['upload'].path, tmp_file)
			block_hash = Digest::SHA256.hexdigest(File.read(tmp_file))
			
			
			if params[:block]['data_type'] == "USE_MAGIC"
				Kernel.system("#{fileInject_path} --file='#{tmp_file}'")
			else
				Kernel.system("#{fileInject_path} --type=#{params[:block]['data_type']} --file='#{tmp_file}'")
			end

			File.delete(tmp_file)
           	flash[:info] = render_to_string :partial => 'block_upload_successful', :locals => { :hash => block_hash }

        rescue Exception => e
			flash[:error] = "An error occurred while uploading your file: #{e.to_s}"
        end

        redirect_to :controller => 'main', :action => 'index'
	end

	def upload_script
		begin
			@detection_script = DetectionScript.find(params[:detection_script]['id'])	
			uploads_path = @detection_script.upload_path

         # Make sure the watched directory exists
         Dir.mkdir(uploads_path) unless File.directory?(uploads_path)

         # Move the temporary file to our watched directory
			FileUtils.install(params[:detection_script]['upload'].path, "#{uploads_path}/#{params[:detection_script]['upload'].original_filename}", :mode => 0755)

			# Run the post installation script
			if !@detection_script.post_command.nil?
				raise "Post installation command failed" unless Kernel.system(@detection_script.post_command)
			end
			
			# All done
         flash[:info] = render_to_string :partial => 'script_upload_successful', :locals => { :detection_script => @detection_script }

	  rescue Errno::EPERM => e
			flash[:error] = "Unable to chmod 0755.  Please make sure apache has the proper permissions for your upload directory #{uploads_path}"	
	  rescue Errno::EACCES => e
			flash[:error] = "The upload directory #{uploads_path} is not writable.  Please make sure apache user has the ability to write to this directory"	
	  rescue Exception => e
			flash[:error] = "An error occurred while uploading your scripting: #{e.to_s}"
	  end

		redirect_to :back
	end

	def script
	end

end
