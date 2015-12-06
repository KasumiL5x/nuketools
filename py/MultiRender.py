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
		self.__panel.addSingleLineInput('Channels', 'rgba')
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
		write_nodes = []
		for curr_node in reversed(nuke.selectedNodes()):
			write_node = nuke.nodes.Write(inputs=[curr_node])
			write_node['file_type'].setValue(file_format)
			write_node['channels'].setValue(self.__panel.value('Channels'))
			new_node = {
				'node': write_node,
				'iteration': iteration,
				'name': curr_node.fullName()
			}
			write_nodes.append(new_node)
			iteration += 1
		#end

		# execute all of the write nodes and delete them
		for curr_node in write_nodes:
			node = curr_node['node']
			for curr_frame in range(start_frame, end_frame+1):
				file_name = '%s%03d.%02d.%s.%s' % (root_path, curr_frame, curr_node['iteration'], curr_node['name'], file_format)
				node['file'].setValue(file_name)
				node['proxy'].setValue(node['file'].getValue())
				nuke.execute(node, curr_frame, curr_frame, 1)
			#end
			nuke.delete(node)
		#end
	#end
#end

mr = MultiRender()
