module SystemStatusHelper
	def status_column(record)
	end

	def list_row_class(record)
		record.status
   end

   def name_column(record)
		if record.name =~ /masterNugget/i 
      	"#{record.name} (#{ link_to 'restart', :controller => 'status', :action => 'restart' })"
      else
      	record.name
      end
   end

end
