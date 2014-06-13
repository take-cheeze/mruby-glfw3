MRuby::Gem::Specification.new 'mruby-glfw3' do |spec|
  spec.author = 'Takeshi Watanabe'
  spec.license = 'MIT'

  add_dependency 'mruby-gles', :github => 'take-cheeze/mruby-gles', :branch => 'glfw3'
  linker.libraries << 'glfw'
end
