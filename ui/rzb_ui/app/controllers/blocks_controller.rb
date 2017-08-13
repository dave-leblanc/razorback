class BlocksController < ApplicationController
	before_filter :session_cleanup, :only => :report

	def delete
		self.find

		if @block.nil?
			flash[:error] = "The block you requested was not found"
			redirect_to :back
		else
			@block.destroy unless @block.nil?
			flash[:info] = "Your block has been deleted"
			redirect_to :controller => 'events', :action => 'index'
		end
		
	end

	def tree
		begin
			@block = Block.find(params[:id])
			render :layout => false

		rescue Exception => e
			flash[:error] = "The block you requested was not found"
			redirect_to :controller => 'main', :action => 'index'
		end
	end
	
	def find
        begin
			if params[:id]
            	@block = Block.find(params[:id])
			elsif params[:hash]
				@block = Block.find_by_hash_digest!(params[:hash])
			end
        rescue
            flash[:error] = "The block you requested was not found"
            redirect_to :controller => 'main', :action => 'index'
        end

	end

    def report
		self.find
    end

	def download
		self.find

		if @block
			send_file(@block.file_path, :filename => "#{@block.hash_digest}#{@block.file_extension}")
		end
	end

	def report_body
		self.find
		render :partial => 'report_body', :layout => false
	end

	def report_header
		self.find
		render :partial => 'report_header', :layout => false
	end
end
