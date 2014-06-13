MRuby::Gem::Specification.new 'mruby-glfw' do |spec|
  spec.author = 'Takeshi Watanabe'
  spec.license = 'MIT'

  add_dependency 'mruby-gles', :github => 'take-cheeze/mruby-gles', :branch => 'dev'
  linker.libraries << 'glfw'
end
