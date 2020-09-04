pipeline {
  agent { 
    dockerfile {
      args '--runtime=nvidia'
    }
  }
  stages {
    stage('build') {
      steps {
        sh '''
          pip install .
        '''
	sh 'whoami'
	sh 'ls ~/.local/share/holodeck/0.3.1/worlds/'
      }
    }
    stage('test') {
      steps {
        sh '''
          py.test
        '''
      }
    }

  }
}
