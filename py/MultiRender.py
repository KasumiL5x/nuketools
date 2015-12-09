import os
import threading
import nuke
import nukescripts

class RenderWriteNode(threading.Thread):
	"""Renders a single node over a range of frames.  This is threaded because NUKE doesn't play nice rendering from callbacks."""
	def __init__(self, write_node, start_frame, end_frame, file_name):
		threading.Thread.__init__(self)
		self.__node = write_node
		self.__start = start_frame
		self.__end = end_frame
		self.__filename = file_name
	#end

	def run(self):
		try:
			self.__node['file'].setValue(self.__filename)
			self.__node['proxy'].setValue(self.__filename)
			nuke.executeInMainThread(nuke.execute, args=(self.__node, self.__start, self.__end, 1), kwargs={'continueOnError': True})
		except Exception as e:
			print 'Warning: %s' % (e)
	#end
#end

class MultiRender(nukescripts.PythonPanel):
	def __init__(self):
		nukescripts.PythonPanel.__init__(self, 'Multi Render', 'com.ngreen.MultiRender')
		self.__setup_knobs()
	#end

	def __render(self):
		# get the start and end frames
		start_frame = int(self.__start_frame_knob.value())
		end_frame = int(self.__end_frame_knob.value())

		# validate frame values
		if (start_frame < 0) or (end_frame < 0):
			print 'Warning: You cannot render negative frames.'
			return
		#end

		# validate frame range
		if start_frame > end_frame:
			print 'Warning: Start frame is greater than end frame.'
			return
		#end

		# ensure the requested path exists
		root_path = self.__output_path_knob.value()
		if not os.path.exists(root_path):
			try:
				os.makedirs(root_path)
			except Exception, e:
				print 'Warning: Tried to create output path but failed. (%s)' % (e)
				return
		#end

		# ensure the output path has a trailing slash so that nuke doesn't shit its pants
		if root_path[-1] != '/':
			root_path += '/'
		#end

		# set proxy enabled based on proxy boolean
		nuke.root().setProxy(self.__use_proxy_knob.value())

		# get the file format
		file_format = self.__file_formats_knob.value()

		# get and validate output channels
		output_channels = self.__output_channels_knob.value()
		if 'none' == output_channels:
			print 'Warning: Output channels must contain some data!'
			return
		#end

		# create and configure each write node for each selected node
		iteration = 0
		write_nodes = []
		for curr_node in reversed(nuke.selectedNodes()):
			write_node = nuke.nodes.Write(inputs=[curr_node])
			write_node['file_type'].setValue(file_format)
			write_node['channels'].setValue(output_channels)
			new_node = {
				'node': write_node,
				'index': iteration,
				'name': curr_node.fullName()
			}
			write_nodes.append(new_node)
			iteration += 1
		#end

		# execute all of the write nodes and delete them
		for curr_node in write_nodes:
			node = curr_node['node']

			# generate a filename (<path><frame number>.<select index>.<node name>.<ext>)
			file_name = '%s###.%03d.%s.%s' % (root_path, curr_node['index'], curr_node['name'], file_format)

			# create and run a new write thread (cannot execute write from here b/c of callbacks)
			write_thread = RenderWriteNode(node, start_frame, end_frame, file_name)
			write_thread.start()

			# remove the now unused node
			nuke.delete(node)
		#end
	#end

	def __setup_knobs(self):
		self.__output_path_knob = nuke.File_Knob('output_path', 'Output Path')
		self.addKnob(self.__output_path_knob)

		self.__file_formats_knob = nuke.Enumeration_Knob('format', 'Format', ['png', 'jpg'])
		self.addKnob(self.__file_formats_knob)

		self.__start_frame_knob = nuke.Int_Knob('start_frame', 'Start Frame')
		self.addKnob(self.__start_frame_knob)

		self.__end_frame_knob = nuke.Int_Knob('end_frame', 'End Frame')
		self.addKnob(self.__end_frame_knob)

		self.__output_channels_knob = nuke.Channel_Knob('output_channels', 'Output Channels')
		self.__output_channels_knob.setFlag(nuke.ENDLINE)
		self.__output_channels_knob.setValue('rgba')
		self.addKnob(self.__output_channels_knob)

		self.__use_proxy_knob = nuke.Boolean_Knob('use_proxy', 'Use Proxy')
		self.__use_proxy_knob.setFlag(nuke.ENDLINE)
		self.addKnob(self.__use_proxy_knob)

		self.__render_button_knob = nuke.PyScript_Knob('render', 'Render')
		self.__render_button_knob.setFlag(nuke.STARTLINE)
		self.addKnob(self.__render_button_knob)

		self.addKnob(nuke.Text_Knob('copyright', 'www.ngreen.org'))
	#end

	def knobChanged(self, knob):
		if knob is self.__render_button_knob:
			self.__render()
	#end
#end

ngreen_mr = MultiRender()
ngreen_mr.show()
