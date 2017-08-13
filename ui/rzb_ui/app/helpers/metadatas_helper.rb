module MetadatasHelper
	def name_column(record)
		if  (record.metadata_name.Name == 'SOURCE') || (record.metadata_name.Name == 'DEST')
			"#{record.metadata_name.Description} #{record.metadata_type.Description}"
		else
			record.metadata_name.Description
		end
		
	end

	def metadata_column(record)
		if record.metadata_name != MetadataName.REPORT

			output = ""
			record.to_s.split(/[\r\n]+/).each do |line|
				if line.size > 100
					output << line.scan(/.{1,100}/).join("<br />") + "\r\n"
				else
					output << line + "\r\n"
				end
			end

			output.gsub(/[\r\n]+/, "<br />&nbsp;<br />")
		else
			link_to "View", { :controller => 'metadatas', :action => 'report', :id => record.id }, :popup => ['Report', 'height=600,width=800']
		end
	end

end
