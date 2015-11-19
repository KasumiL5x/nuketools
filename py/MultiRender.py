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
		self.__panel.addSingleLineInput('Frame', nuke.frame())
		self.__panel.addButton('Cancel')
		self.__panel.addButton('Render')
		self.__dialog_result = self.__panel.show()
	#end

	def __process_input(self):
		if self.__dialog_result != 1: # 1 = render button
			return
		#end

		# validate the input frame number
		frame = self.__panel.value('Frame')
		if (not frame.isdigit()):
			print 'Warning: Input frame is not valid.'
			return
		#end
		frame = int(frame)

		# ensure that the path exists
		root_path = self.__panel.value('Output path')
		if not os.path.isdir(root_path):
			print 'Warning: Path is not valid.'
			return
		#end

		# ensure the path has a trailing slash
		if root_path[-1] != '/':
			root_path += '/'
		#end

		# get the file format
		file_format = self.__panel.value('Format')

		# create and configure a write node for each selected node
		iteration = 0
		write_nodes = []
		for curr_node in nuke.selectedNodes():
			write_node = nuke.nodes.Write(inputs=[curr_node])
			write_node['file_type'].setValue(file_format)
			write_node['file'].setValue(root_path + str(iteration) + '_' + curr_node.fullName() + '.' + file_format)
			write_nodes.append(write_node)
			iteration += 1
		#end

		# execute all of the write nodes and delete them
		for curr_node in write_nodes:
				nuke.execute(curr_node, frame, frame, 1)
				nuke.delete(curr_node)
		#end
	#end
#end

mr = MultiRender()
