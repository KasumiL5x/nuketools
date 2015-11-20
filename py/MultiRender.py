import os
import nuke

class MultiRender():
	def __init__(self):
		self.__build_gui()
		self.__process_input()
	#end

	def __build_gui(self):
		self.__panel = nuke.Panel('Multi Render - Daniel Green')
		self.__panel.setWidth(400)
		self.__panel.addFilenameSearch('Output path', '')
		self.__panel.addEnumerationPulldown('Format', 'jpg png')
		self.__panel.addSingleLineInput('Start Frame', nuke.frame())
		self.__panel.addSingleLineInput('End Frame', nuke.frame())
		self.__panel.addBooleanCheckBox('Use Proxy', nuke.root().proxy())
		self.__panel.addButton('Cancel')
		self.__panel.addButton('Render')
		self.__dialog_result = self.__panel.show()
	#end

	def __process_input(self):
		if self.__dialog_result != 1: # 1 = render button
			return
		#end

		# validate the start frame
		start_frame = self.__panel.value('Start Frame')
		if (not start_frame.isdigit()):
			print 'Warning: Start frame is not valid.'
			return
		#end
		start_frame = int(start_frame)

		# validate the end frame
		end_frame = self.__panel.value('End Frame')
		if not end_frame.isdigit():
			print 'Warning: End frame is not valid.'
			return
		#end
		end_frame = int(end_frame)

		# validate frame range
		if start_frame > end_frame:
			print 'Warning: Start frame is greater than end frame.'
			return
		#end

		# ensure that the path exists
		root_path = self.__panel.value('Output path')
		if not os.path.exists(root_path):
			try:
				os.makedirs(root_path)
			except Exception, e:
				print 'Warning: Tried to create path but failed.'
				print e
				return
		#end

		# ensure the path has a trailing slash
		if root_path[-1] != '/':
			root_path += '/'
		#end

		# set root configuration to match proxy
		nuke.root().setProxy(self.__panel.value('Use Proxy'))

		# get the file format
		file_format = self.__panel.value('Format')

		# create and configure a write node for each selected node
		iteration = 0
		write_nodes = [] # [[Write, iteration, node name],...]
		for curr_node in reversed(nuke.selectedNodes()):
			write_node = nuke.nodes.Write(inputs=[curr_node])
			write_node['file_type'].setValue(file_format)
			write_node['file'].setValue(root_path + str(iteration) + '_' + curr_node.fullName() + '.' + file_format)
			write_node['proxy'].setValue(write_node['file'].getValue())
			write_nodes.append([write_node, str(iteration), curr_node.fullName()])
			iteration += 1
		#end

		# execute all of the write nodes and delete them
		
		for curr_node in write_nodes:
			# write all requested frames
			for curr_frame in range(start_frame, end_frame+1):
				curr_node[0]['file'].setValue(root_path + str(curr_frame) + '-' + curr_node[1] + '-' + curr_node[2] + '.' + file_format)
				curr_node[0]['proxy'].setValue(curr_node[0]['file'].getValue())
				nuke.execute(curr_node[0], curr_frame, curr_frame, 1) # cannot render ranges?
			#end
			nuke.delete(curr_node[0])
			
		#end
	#end
#end

mr = MultiRender()
