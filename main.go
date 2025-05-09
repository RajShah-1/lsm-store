package main

import (
	"fmt"
	"github.com/RajShah-1/geokv/kv"
)

func main() {
	store := kv.NewStore("data/very-simple.db")

	if store.Set("name", "Raj") {
		fmt.Println("Set name to Raj")
	} else {
		fmt.Println("Failed to set name")
	}
	
	if store.Set("language", "Go") {
		fmt.Println("Set language to Go")
	} else {
		fmt.Println("Failed to set language")
	}

	val, _ := store.Get("name")
	fmt.Println("Got:", val)
}
