package kv

import (
	"bufio"
	"fmt"
	"os"
	"strings"
)

type Store struct {
	file *os.File
	data map[string]string
}

func NewStore(path string) *Store {
	file, err := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_RDWR, 0644)
	if err != nil {
		panic(err)
	}

	store := &Store{
		file: file,
		data: make(map[string]string),
	}

	store.loadFromDisk()
	return store
}

func (s *Store) loadFromDisk() {
	s.file.Seek(0, 0)
	scanner := bufio.NewScanner(s.file)
	for scanner.Scan() {
		line := scanner.Text()
		parts := strings.SplitN(line, "=", 2)
		if len(parts) == 2 {
			key, value := parts[0], parts[1]
			s.data[key] = value
		}
	}
}

func (s *Store) Set(key, value string) (bool) {
	entry := fmt.Sprintf("%s=%s\n", key, value)
	_, err := s.file.WriteString(entry)
	if err != nil {
		return false
	}
	s.data[key] = value
	return true
}

func (s *Store) Get(key string) (string, bool) {
	val, ok := s.data[key]
	return val, ok
}
