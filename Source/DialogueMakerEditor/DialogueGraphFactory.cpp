// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueGraphFactory.h"

#include "DialogueEdGraphNode.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueMaker/DialogueGraph.h"
#include "Kismet2/BlueprintEditorUtils.h"

UDialogueGraphFactory::UDialogueGraphFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SupportedClass = UDialogueGraph::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDialogueGraphFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
                                                 UObject* Context, FFeedbackContext* Warn)
{
	UDialogueGraph* NewAsset = NewObject<UDialogueGraph>(InParent, InName, Flags);

	// Editor Graph 생성 및 할당
	// NewAsset->Graph = Cast<UDialogueRuntimeGraph>(FBlueprintEditorUtils::CreateNewGraph(NewAsset, NAME_None, UDialogueEdGraph::StaticClass(), UDialogueEdGraphSchema::StaticClass()));
	// NewAsset->Graph->bAllowDeletion = false;

	// FEdGraphSchemaAction_NewNode를 사용해 첫 노드를 생성하고 그래프에 추가
	// UEdGraph* Graph = NewAsset->Graph;
	// const UEdGraphSchema* Schema = Graph->GetSchema();
	// FGraphNodeCreator<UDialogueEdGraphNode> NodeCreator(*Graph);
	// UDialogueEdGraphNode* RootNode = NodeCreator.CreateNode();
	// RootNode->Dialogue.DialogueText = FText::FromString(TEXT("시작 대화")); // 기본 텍스트 설정
	// Schema->SetNodePosition(RootNode, FVector2D(0, 0));
	// NodeCreator.Finalize();

	// Graph->AddNode(RootNode);
	
	return NewAsset;
}
